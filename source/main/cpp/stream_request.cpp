#include "ccore/c_allocator.h"
#include "ccore/c_math.h"
#include "ccore/c_memory.h"
#include "ccore/c_qsort.h"
#include "ccore/c_vmem.h"
#include "cbase/c_runes.h"

#include "cconartist/stream_request.h"
#include "cconartist/stream_manager.h"
#include "cconartist/job_manager.h"

#include "cmmio/c_mmio.h"
#include "clibuv/uv.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace ncore
{
    // We want a manager that can create new streams on disk, and we want this to be on a separate
    // manager since we don't want to block the main event loop when creating new streams.
    // It also monitors a specific file that contains mappings [id => name] and keeps
    // reloading it when it changes on disk.
    // When a new stream is requested, it creates the file on disk when the mapping exists.
    // If possible, in the UI we do want to see the list of active stream requests, so we can know
    // which streams to registers in the mapping file.
    // The main event loop communicates by using a 'channel' to push a pointer to a stream request
    // that contains the user-id, stream-id, and resulting mappedfile, and waits for a response on
    // the receive channel for stream requests that have been processed.

    struct stream_request_t
    {
        i32                  m_mapping_index;  // Index into the mappings array
        i32                  m_version;        // Version of the mappings file
        stream_id_t          m_stream_id;
        u64                  m_user_id;
        u64                  m_mmfile_size;
        nmmio::mappedfile_t* m_mmfile;
    };

#define DMAPPING_NAME_MAXLEN 64
    struct stream_mappings_t
    {
        i32         m_version;
        char*       m_file_content;
        i32         m_file_content_size;
        i32         m_file_content_capacity;
        const char* m_mappings_filepath;
        struct stat m_mappings_file_stat;
        u64*        m_mapping_ids;
        char*       m_mapping_names;
        i32*        m_mappings_sorted;
        i32         m_mappings_size;
        i32         m_mappings_capacity;
    };

    static i32 find_mapping(stream_mappings_t* mappings, u64 user_id)
    {
        // Use the sorted array to do a binary search
        i32 left  = 0;
        i32 right = mappings->m_mappings_size - 1;
        while (left <= right)
        {
            const i32 mid       = (left + right) / 2;
            const i32 mid_index = mappings->m_mappings_sorted[mid];
            const u64 mid_id    = mappings->m_mapping_ids[mid_index];
            if (user_id < mid_id)
            {
                right = mid - 1;
            }
            else if (user_id > mid_id)
            {
                left = mid + 1;
            }
            else
            {
                return mid_index;
            }
        }
        return -1;
    }

    static void add_mapping(stream_mappings_t* mappings, u64 user_id, const char* name)
    {
        char* name_dest = &mappings->m_mapping_names[mappings->m_mappings_size * DMAPPING_NAME_MAXLEN];
        nmem::memcpy(name_dest, name, DMAPPING_NAME_MAXLEN);
        mappings->m_mapping_ids[mappings->m_mappings_size] = user_id;
        mappings->m_mappings_size++;
    }

    static s8 mapping_cmp_fn(const void* lhs, const void* rhs, const void* stream_mapping)
    {
        stream_mappings_t* mappings = (stream_mappings_t*)stream_mapping;

        const i32 lhs_index = *(i32 const*)lhs;
        const i32 rhs_index = *(i32 const*)rhs;
        const u64 lhs_id    = mappings->m_mapping_ids[lhs_index];
        const u64 rhs_id    = mappings->m_mapping_ids[rhs_index];
        if (lhs_id < rhs_id)
            return -1;
        if (lhs_id > rhs_id)
            return 1;
        return 0;
    }

    static void sort_mappings(stream_mappings_t* mappings)
    {
        // This sorts the array of indices that index into the mappings arrays
        nsort::sort<i32>(mappings->m_mappings_sorted, (u32)mappings->m_mappings_size, mapping_cmp_fn, mappings);
    }

    struct stream_request_manager_t
    {
        alloc_t*           m_allocator;
        job_manager_t*     m_job_manager;
        job_channel_t      m_mappings_channel;        // Channel to push stream mapping to
        job_channel_t      m_stream_request_channel;  // Channel to push stream requests to
        const char*        m_streams_basepath;
        f64                m_last_mappings_check_time;
        stream_mappings_t* m_loaded_mappings;
        stream_mappings_t* m_mappings;
        stream_request_t*  m_requests;
        i32                m_requests_capacity;
        i16*               m_free_requests;
        i16*               m_active_requests;
        i16*               m_done_requests;
        i32                m_free_requests_size;
        i32                m_active_requests_size;
        i32                m_done_requests_size;
    };

    stream_request_manager_t* create_stream_request_manager(alloc_t* allocator, job_manager_t* jm, f64 now, const char* streams_basepath, const char* mappings_filepath)
    {
        stream_request_manager_t* manager   = g_allocate<stream_request_manager_t>(allocator);
        manager->m_allocator                = allocator;
        manager->m_job_manager              = jm;
        manager->m_streams_basepath         = g_duplicate_string(allocator, streams_basepath);
        manager->m_free_requests_size       = 0;
        manager->m_active_requests_size     = 0;
        manager->m_done_requests_size       = 0;
        manager->m_requests_capacity        = 256;
        manager->m_requests                 = g_allocate_array<stream_request_t>(allocator, manager->m_requests_capacity);
        manager->m_free_requests            = g_allocate_array<i16>(allocator, manager->m_requests_capacity);
        manager->m_active_requests          = g_allocate_array<i16>(allocator, manager->m_requests_capacity);
        manager->m_last_mappings_check_time = now;

        for (i32 i = 0; i < 2; ++i)
        {
            stream_mappings_t* mappings       = g_allocate<stream_mappings_t>(allocator);
            mappings->m_file_content_capacity = 64 * cKB;
            mappings->m_file_content          = g_allocate_array<char>(allocator, mappings->m_file_content_capacity);
            mappings->m_file_content_size     = 0;
            mappings->m_mappings_filepath     = g_duplicate_string(allocator, mappings_filepath);
            mappings->m_mappings_size         = 0;
            mappings->m_mappings_capacity     = 1024;
            mappings->m_mapping_ids           = g_allocate_array<u64>(allocator, mappings->m_mappings_capacity);
            mappings->m_mapping_names         = g_allocate_array<char>(allocator, mappings->m_mappings_capacity * DMAPPING_NAME_MAXLEN);
            mappings->m_mappings_sorted       = g_allocate_array<i32>(allocator, mappings->m_mappings_capacity);
            memset(&mappings->m_mappings_file_stat, 0, sizeof(struct stat));
            switch (i)
            {
                case 0: manager->m_loaded_mappings = mappings; break;
                case 1: manager->m_mappings = mappings; break;
            }
        }

        manager->m_mappings_channel       = init_channel(manager->m_job_manager, 2);
        manager->m_stream_request_channel = init_channel(manager->m_job_manager, 256);

        return manager;
    }

    void destroy_stream_request_manager(stream_request_manager_t*& manager)
    {
        // Deallocate mappings
        g_deallocate_string(manager->m_allocator, manager->m_mappings->m_mappings_filepath);
        g_deallocate_array<u64>(manager->m_allocator, manager->m_mappings->m_mapping_ids);
        g_deallocate_array<char>(manager->m_allocator, manager->m_mappings->m_mapping_names);
        g_deallocate<stream_mappings_t>(manager->m_allocator, manager->m_mappings);

        // Deallocate loaded mappings
        g_deallocate_string(manager->m_allocator, manager->m_loaded_mappings->m_mappings_filepath);
        g_deallocate_array<u64>(manager->m_allocator, manager->m_loaded_mappings->m_mapping_ids);
        g_deallocate_array<char>(manager->m_allocator, manager->m_loaded_mappings->m_mapping_names);
        g_deallocate<stream_mappings_t>(manager->m_allocator, manager->m_loaded_mappings);

        // Deallocate members of manager
        g_deallocate_string(manager->m_allocator, manager->m_streams_basepath);
        g_deallocate_array<stream_request_t>(manager->m_allocator, manager->m_requests);
        g_deallocate_array<i16>(manager->m_allocator, manager->m_active_requests);
        g_deallocate<stream_request_manager_t>(manager->m_allocator, manager);

        // Nullify pointer
        manager = nullptr;
    }

    void push_stream_request(stream_request_manager_t* srm, u64 user_id, stream_id_t stream_id, u64 mmfile_size, nmmio::mappedfile_t* mmfile)
    {
        if (srm->m_free_requests_size > 0)
        {
            const i16         request_index = srm->m_free_requests[--srm->m_free_requests_size];
            stream_request_t* req           = &srm->m_requests[request_index];
            req->m_user_id                  = user_id;
            req->m_mmfile_size              = mmfile_size;
            req->m_mapping_index            = -1;
            req->m_version                  = srm->m_mappings->m_version - 1;
            req->m_stream_id                = stream_id;
            req->m_mmfile                   = mmfile;

            // Move to active requests
            srm->m_active_requests[srm->m_active_requests_size++] = request_index;
        }
    }

    bool pop_stream_request(stream_request_manager_t* srm, u64& user_id, nmmio::mappedfile_t*& out_mmfile)
    {
        if (srm->m_done_requests_size > 0)
        {
            const i16         request_index = srm->m_done_requests[--srm->m_done_requests_size];
            stream_request_t* req           = &srm->m_requests[request_index];
            user_id                         = req->m_user_id;
            out_mmfile                      = req->m_mmfile;

            // Reclaim request slot
            srm->m_free_requests[srm->m_free_requests_size++] = request_index;
            return true;
        }
        return false;
    }

    // Called from main event loop to update stream requests and mappings
    void update_mappings_job_fn(void* arg0, void* arg1);
    void stream_request_fn(void* arg0, void* arg1);

    void update_stream_requests(stream_request_manager_t* srm, f64 now)
    {
        // Every 10 seconds we want to check the mappings file for changes
        if (srm->m_loaded_mappings != nullptr)
        {
            if (srm->m_last_mappings_check_time + 10.0 < now)
            {
                srm->m_last_mappings_check_time = now;
                stream_mappings_t* mappings     = srm->m_loaded_mappings;
                srm->m_loaded_mappings          = nullptr;
                push_job(srm->m_job_manager, srm->m_mappings_channel, update_mappings_job_fn, mappings, nullptr);
            }
        }

        // Check for jobs that are finished

        // Mappings loaded job.
        // When this job comes back, we iterate over all the user-id/name items and add them to the main
        // mappings array if they don't exist yet.
        void* job_data0;
        void* job_data1;
        while (pop_job(srm->m_job_manager, srm->m_mappings_channel, job_data0, job_data1) == 0)
        {
            // Stream mapping job finished
            stream_mappings_t* loaded_mappings = (stream_mappings_t*)job_data1;
            srm->m_loaded_mappings             = loaded_mappings;

            // For each user-id / name, we add them to srm->m_mappings
            const i32 old_mappings_size = srm->m_mappings->m_mappings_size;
            for (i32 i = 0; i < loaded_mappings->m_mappings_size; ++i)
            {
                u64   user_id = loaded_mappings->m_mapping_ids[i];
                char* name    = &loaded_mappings->m_mapping_names[i * DMAPPING_NAME_MAXLEN];
                if (find_mapping(srm->m_mappings, user_id) >= 0)
                    continue;
                add_mapping(srm->m_mappings, user_id, name);
            }
            if (old_mappings_size < srm->m_mappings->m_mappings_size)
            {
                sort_mappings(srm->m_mappings);
            }
        }

        // Stream request jobs
        while (pop_job(srm->m_job_manager, srm->m_stream_request_channel, job_data0, job_data1) == 0)
        {
            // Stream request job finished
            stream_request_t* request = (stream_request_t*)job_data0;

            // Push it in the done requests array
            srm->m_done_requests[srm->m_done_requests_size++] = (i16)(request - srm->m_requests);
        }

        // Check stream requests against the mappings
        for (i32 i = 0; i < srm->m_active_requests_size; ++i)
        {
            const i16 request_index = srm->m_active_requests[i];
            if (srm->m_mappings != nullptr && srm->m_requests[request_index].m_mmfile == nullptr)
            {
                // Check if the mapping exists
                const i32 mapping_index = find_mapping(srm->m_mappings, srm->m_requests[request_index].m_user_id);
                if (mapping_index >= 0)
                {
                    stream_request_t* req_to_process = &srm->m_requests[request_index];
                    // Mapping exists, initialize the stream request and push it to the job manager
                    req_to_process->m_mapping_index = mapping_index;
                    req_to_process->m_version       = srm->m_mappings->m_version;

                    push_job(srm->m_job_manager, srm->m_stream_request_channel, stream_request_fn, srm, req_to_process);
                }
            }
        }
    }

    void update_mappings_job_fn(void* arg0, void* arg1)
    {
        stream_mappings_t* mappings = (stream_mappings_t*)arg0;
        CC_UNUSED(arg1);

        FILE* file = fopen(mappings->m_mappings_filepath, "rb");
        if (file != nullptr)
        {
            struct stat current_stat;
            if (stat(mappings->m_mappings_filepath, &current_stat) == 0)
            {
                if (current_stat.st_mtime != mappings->m_mappings_file_stat.st_mtime)
                {
                    // File changed, reload
                    fseek(file, 0, SEEK_END);
                    i32 file_size = (i32)ftell(file);
                    fseek(file, 0, SEEK_SET);

                    // Guard against too large files
                    if (file_size <= mappings->m_file_content_capacity)
                    {
                        fread(mappings->m_file_content, 1, file_size, file);
                        mappings->m_file_content_size = file_size;

                        // Parse the mappings, which are line based and each line is 'ID=filename'
                        mappings->m_mappings_size = 0;
                        ncore::nrunes::reader_t reader(mappings->m_file_content, mappings->m_file_content_size);
                        while (!reader.end())
                        {
                            crunes_t line = ncore::nrunes::read_line(&reader);
                            crunes_t left, right;
                            if (nrunes::selectLeftAndRightOf(line, '=', left, right))
                            {
                                i32 mapping_index = mappings->m_mappings_size++;
                                // left = ID, which is '001122334455' or '00:11:22:33:44:55' format
                                mappings->m_mapping_ids[mapping_index] = nrunes::parse_mac(left);
                                // right = filename, max 255-8 characters
                                const u32 rlen = math::min((u32)(right.m_end - right.m_str), (u32)DMAPPING_NAME_MAXLEN - 1);
                                nmem::memcpy(&mappings->m_mapping_names[mapping_index * DMAPPING_NAME_MAXLEN], right.m_ascii + right.m_str, rlen);
                                mappings->m_mapping_names[mapping_index * DMAPPING_NAME_MAXLEN + rlen] = 0;
                            }
                        }
                        mappings->m_version++;  // Increment version on each change, so that requests can know if they are outdated
                        mappings->m_mappings_file_stat = current_stat;
                    }
                }
            }
            fclose(file);
        }
    }

    void stream_request_fn(void* arg0, void* arg1)
    {
        stream_request_manager_t* srm           = (stream_request_manager_t*)arg0;
        stream_request_t*         known_request = (stream_request_t*)arg1;

        const i32 mapping_index = known_request->m_mapping_index;

        const char* name             = &srm->m_mappings->m_mapping_names[mapping_index * DMAPPING_NAME_MAXLEN];
        const char* streams_basepath = srm->m_streams_basepath;

        // Create the stream file on disk
        char filepath[MAXPATHLEN];
        snprintf(filepath, sizeof(filepath), "%s/%s.rwstream", streams_basepath, name);

        // Create the mapped file
        nmmio::mappedfile_t* mmfile_rw = known_request->m_mmfile;
        if (nmmio::create_rw(mmfile_rw, filepath, known_request->m_mmfile_size))
        {
            known_request->m_mmfile = mmfile_rw;
        }
    }

}  // namespace ncore
