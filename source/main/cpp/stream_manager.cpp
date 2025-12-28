#include "ccore/c_allocator.h"
#include "ccore/c_math.h"
#include "ccore/c_memory.h"
#include "ccore/c_arena.h"
#include "cbase/c_runes.h"

#include "cconartist/stream_manager.h"
#include "cconartist/stream_id_registry.h"
#include "cconartist/channel.h"

#include "cmmio/c_mmio.h"

#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>

namespace ncore
{
    // Notes:
    // - Stream Files are not to be used accross different platforms with different endianness
    // - All is not thread safe, the user must ensure proper locking if multiple threads access this
    // - stream_manager_t::update ?, where every night it analyzes the streams and finalize/rotates them if needed.
    // - file naming convention: {name}_XXXX.stream, where XXXX is a 4 digit incrementing number.

    namespace estream_mode
    {
        typedef u8 enum_t;
        enum
        {
            readonly  = 1,
            readwrite = 2,
        };
    }  // namespace estream_mode

    static nstreamtype::enum_t s_get_stream_type(stream_id_t sid) { return (nstreamtype::enum_t)((sid >> 24) & 0xff); }
    static u16                 s_get_stream_index(stream_id_t sid) { return (u16)(sid & 0xffff); }

    struct stream_header_t
    {
        u64 m_user_id;       // Stream identifier (mostly a MAC address or similar unique identifier)
        u16 m_user_index;    // Index of the stream (in case multiple (historical) streams exist with the same user_id)
        u16 m_stream_type;   // (stream_type_t) Type of stream
        u16 m_reserved0;     // Type of the stream (sensor type, audio, video, etc)
        u16 m_reserved1;     // Reserved for future use
        u32 m_sizeof_item;   // Size of each item (bytes) in the stream (for fixed size streams)
        u32 m_reserved2;     // Reserved for future use
        u64 m_time_begin;    // Time of the first item in the stream
        u64 m_stream_size;   // Size of the stream file in bytes
        u64 m_item_count;    // Number of items in the stream
        u64 m_time_end;      // Time of the last item in the stream
        u64 m_write_cursor;  // Cursor of where the next data will be written in the stream
    };

    struct stream_manager_t
    {
        char*                   m_base_path;
        time_t                  m_last_update_time;
        alloc_t*                m_allocator;
        stream_id_registry_t*   m_stream_id_registry;
        i32                     m_num_ro_streams;
        i32                     m_max_ro_streams;
        nmmio::mappedfile_t**   m_ro_stream_files;
        const stream_header_t** m_ro_streams;
        const stream_header_t** m_ro_streams_sorted;  // For quick lookup by user_id
        u32                     m_num_rw_streams;
        u32                     m_max_rw_streams;
        char**                  m_rw_stream_filepaths;
        void**                  m_rw_stream_memory;
        nmmio::mappedfile_t**   m_rw_stream_files;
        stream_header_t**       m_rw_streams;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    static bool has_ro_extension(const char* filename)
    {
        const char* ext = strrchr(filename, '.');
        return (ext && strcmp(ext, ".rostream") == 0);
    }

    static bool has_rw_extension(const char* filename)
    {
        const char* ext = strrchr(filename, '.');
        return (ext && strcmp(ext, ".rwstream") == 0);
    }

    void stream_manager_resize_ro(stream_manager_t* m)
    {
        if (m->m_num_ro_streams >= m->m_max_ro_streams)
        {
            // Resize the read-only arrays
            i32                     new_max_ro_streams    = m->m_max_ro_streams * 2;
            nmmio::mappedfile_t**   new_ro_stream_files   = g_reallocate_array<nmmio::mappedfile_t*>(m->m_allocator, m->m_ro_stream_files, m->m_max_ro_streams, new_max_ro_streams);
            const stream_header_t** new_ro_streams        = g_reallocate_array<const stream_header_t*>(m->m_allocator, m->m_ro_streams, m->m_max_ro_streams, new_max_ro_streams);
            const stream_header_t** new_ro_streams_sorted = g_reallocate_array<const stream_header_t*>(m->m_allocator, m->m_ro_streams_sorted, m->m_max_ro_streams, new_max_ro_streams);
            m->m_ro_stream_files                          = new_ro_stream_files;
            m->m_ro_streams                               = new_ro_streams;
            m->m_ro_streams_sorted                        = new_ro_streams_sorted;
            m->m_max_ro_streams                           = new_max_ro_streams;
        }
    }

    void stream_manager_resize_rw(stream_manager_t* m)
    {
        if (m->m_num_rw_streams >= m->m_max_rw_streams)
        {
            // Resize the read-write arrays
            i32                   new_max_rw_streams      = m->m_max_rw_streams * 2;
            char**                new_rw_stream_filepaths = g_reallocate_array<char*>(m->m_allocator, m->m_rw_stream_filepaths, m->m_max_rw_streams, new_max_rw_streams);
            nmmio::mappedfile_t** new_rw_stream_files     = g_reallocate_array<nmmio::mappedfile_t*>(m->m_allocator, m->m_rw_stream_files, m->m_max_rw_streams, new_max_rw_streams);
            stream_header_t**     new_rw_streams          = g_reallocate_array<stream_header_t*>(m->m_allocator, m->m_rw_streams, m->m_max_rw_streams, new_max_rw_streams);
            m->m_rw_stream_filepaths                      = new_rw_stream_filepaths;
            m->m_rw_stream_files                          = new_rw_stream_files;
            m->m_rw_streams                               = new_rw_streams;
            m->m_max_rw_streams                           = new_max_rw_streams;
        }
    }

    void stream_manager_add_ro_stream(stream_manager_t* m, const char* filepath)
    {
        stream_manager_resize_ro(m);

        // Open the mapped file
        nmmio::mappedfile_t* mmfile_ro = nullptr;
        nmmio::allocate(m->m_allocator, mmfile_ro);
        if (nmmio::open_ro(mmfile_ro, filepath))
        {
            // Get the stream header
            const stream_header_t* header = (stream_header_t*)nmmio::address_ro(mmfile_ro);
            if (header != nullptr)
            {
                // Register the read-only stream
                m->m_ro_streams[m->m_num_ro_streams]        = header;
                m->m_ro_streams_sorted[m->m_num_ro_streams] = header;
                m->m_ro_stream_files[m->m_num_ro_streams]   = mmfile_ro;
                m->m_num_ro_streams += 1;
                return;
            }
            nmmio::close(mmfile_ro);
        }
        nmmio::deallocate(m->m_allocator, mmfile_ro);
    }

    // Find the largest user_index for the given user_id in the read-write streams
    // This means that if there are multiple streams for the same user_id, and they can
    // exist as read-only stream in sub folders, we need to find the largest user_index to assign
    // to a new read-write stream.
    static u16 stream_manager_largest_user_index_for(stream_manager_t* m, u64 user_id)
    {
        u16 largest_index = 0;
        for (i32 i = 0; i < m->m_num_ro_streams; i++)
        {
            const stream_header_t* header = m->m_ro_streams[i];
            if (header->m_user_id == user_id)
            {
                if (header->m_user_index > largest_index)
                {
                    largest_index = header->m_user_index;
                }
            }
        }
        return largest_index;
    }

    void stream_manager_add_rw_stream(stream_manager_t* m, const char* filepath)
    {
        stream_manager_resize_rw(m);

        // Open the mapped file
        nmmio::mappedfile_t* mmfile_rw = nullptr;
        nmmio::allocate(m->m_allocator, mmfile_rw);
        if (nmmio::open_rw(mmfile_rw, filepath))
        {
            // Get the stream header
            stream_header_t* header = (stream_header_t*)nmmio::address_rw(mmfile_rw);
            if (header != nullptr)
            {
                // Register the read-write stream
                m->m_rw_stream_filepaths[m->m_num_rw_streams] = g_allocate_array<char>(m->m_allocator, strlen(filepath) + 1);
                strlcpy((char*)m->m_rw_stream_filepaths[m->m_num_rw_streams], filepath, strlen(filepath) + 1);
                m->m_rw_stream_files[m->m_num_rw_streams] = mmfile_rw;
                m->m_rw_streams[m->m_num_rw_streams]      = header;
                m->m_num_rw_streams += 1;
                return;
            }
            nmmio::close(mmfile_rw);
        }
        nmmio::deallocate(m->m_allocator, mmfile_rw);
    }

    // Scan the base path and register any read-only or read-write stream files found.
    // This will collect *.rwstream files in the root of `m->m_base_path` and also
    // scan one directory level deep for numeric directories containing *.rostream files.
    static bool s_dir_is_current_or_parent(const char* name) { return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0); }

    static void stream_manager_scan_basepath(stream_manager_t* m)
    {
        char full_path[MAXPATHLEN];

        DIR* dir = opendir(m->m_base_path);
        if (!dir)
            return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            // Skip current/parent entries
            if (s_dir_is_current_or_parent(entry->d_name))
                continue;

            const bool is_dir = (entry->d_type == DT_DIR);

            // Root-level read-write streams
            if (!is_dir)
            {
                if (has_rw_extension(entry->d_name))
                {
                    snprintf(full_path, sizeof(full_path), "%s/%s", m->m_base_path, entry->d_name);
                    stream_manager_add_rw_stream(m, full_path);
                }
                continue;
            }

            snprintf(full_path, sizeof(full_path), "%s/%s", m->m_base_path, entry->d_name);
            DIR* subdir = opendir(full_path);
            if (!subdir)
                continue;

            struct dirent* subentry;
            char           sub_full[MAXPATHLEN];
            while ((subentry = readdir(subdir)) != nullptr)
            {
                if (subentry->d_type == DT_DIR)
                    continue;
                if (has_ro_extension(subentry->d_name))
                {
                    snprintf(sub_full, sizeof(sub_full), "%s/%s", full_path, subentry->d_name);
                    stream_manager_add_ro_stream(m, sub_full);
                }
            }
            closedir(subdir);
        }
        closedir(dir);
    }

    stream_manager_t* stream_manager_create(alloc_t* allocator, i32 max_streams, const char* base_path)
    {
        ASSERT(allocator != nullptr);
        ASSERT(max_streams > 0);
        ASSERT(base_path != nullptr);

        // Create the stream manager
        //   - Create arrays for read-only and read-write streams

        stream_manager_t* m = g_construct<stream_manager_t>(allocator);
        m->m_base_path      = g_allocate_array<char>(allocator, strlen(base_path) + 1);
        strcpy((char*)m->m_base_path, base_path);
        m->m_last_update_time    = time(nullptr);  // Current time
        m->m_allocator           = allocator;
        m->m_stream_id_registry  = stream_id_registry_create(allocator, max_streams);
        m->m_num_ro_streams      = 0;
        m->m_max_ro_streams      = max_streams;
        m->m_ro_streams          = g_allocate_array_and_clear<const stream_header_t*>(allocator, max_streams);
        m->m_ro_streams_sorted   = g_allocate_array_and_clear<const stream_header_t*>(allocator, max_streams);
        m->m_num_rw_streams      = 0;
        m->m_max_rw_streams      = max_streams;
        m->m_rw_stream_filepaths = g_allocate_array_and_clear<char*>(allocator, max_streams);
        m->m_rw_stream_files     = g_allocate_array_and_clear<nmmio::mappedfile_t*>(allocator, max_streams);
        m->m_rw_streams          = g_allocate_array_and_clear<stream_header_t*>(allocator, max_streams);

        // Scan base path and register streams
        stream_manager_scan_basepath(m);

        return m;
    }

    void stream_manager_flush(stream_manager_t* manager)
    {
        // Flush all read-write streams to disk
        for (u32 i = 0; i < manager->m_num_rw_streams; i++)
        {
            nmmio::mappedfile_t* rw_file = manager->m_rw_stream_files[i];
            if (rw_file != nullptr)
            {
                nmmio::sync(rw_file);
            }
        }
    }

    void stream_manager_update(stream_manager_t* manager, f64 now)
    {
        // Check time since last update and skip if too soon
        //  - We only want to do this once in a while, e.g. once every day
        // Update the stream manager
        //  - Check if any streams need to 'grow'
        //    We can do this by 'closing' the stream, and opening it again with a larger size.
        // Using m_time_begin and m_time_end together with m_item_count we can determine the throughput
        // and determine how many days a stream still has to go before it should be extended in size.

        // Check the stream request channel for any completed stream requests and process them.
        // A completed stream request provides us with an opened mmapped file that we can use
        // to back the read-write stream instead of the memory stream we have been using so far.
        // - Copy the memory stream contents to the mmapped file
        // - Update the stream header in the stream manager
        // - Update the filename of the read-write stream in the stream manager
        // - Deallocate the memory stream and set its pointer to nullptr
        // - Set the mapped file pointer in the stream manager
    }

    void stream_manager_destroy(alloc_t* allocator, stream_manager_t*& manager)
    {
        // Destroy the stream manager
        //  - Flush and close all open streams
        //  - Deallocate all memory used by the stream manager

        // Close all read-write streams
        for (u32 i = 0; i < manager->m_num_rw_streams; i++)
        {
            nmmio::mappedfile_t* rw_file = manager->m_rw_stream_files[i];
            if (rw_file != nullptr)
            {
                nmmio::sync(rw_file);
                nmmio::close(rw_file);
                nmmio::deallocate(manager->m_allocator, rw_file);
                manager->m_rw_stream_files[i] = nullptr;
            }
        }
        g_deallocate_array<char*>(allocator, manager->m_rw_stream_filepaths);
        g_deallocate_array<nmmio::mappedfile_t*>(allocator, manager->m_rw_stream_files);
        g_deallocate_array<stream_header_t*>(allocator, manager->m_rw_streams);

        // Close all read-only streams
        for (i32 i = 0; i < manager->m_num_ro_streams; i++)
        {
            nmmio::mappedfile_t* ro_file = manager->m_ro_stream_files[i];
            if (ro_file != nullptr)
            {
                nmmio::close(ro_file);
                nmmio::deallocate(manager->m_allocator, ro_file);
                manager->m_ro_stream_files[i] = nullptr;
            }
        }

        g_deallocate_array<nmmio::mappedfile_t*>(allocator, manager->m_ro_stream_files);
        g_deallocate_array<const stream_header_t*>(allocator, manager->m_ro_streams);
        g_deallocate_array<const stream_header_t*>(allocator, manager->m_ro_streams_sorted);

        g_deallocate_array<char>(allocator, manager->m_base_path);

        g_destruct(allocator, manager);
    }

    static u8* stream_write_u64_le(u8* dest, u64 value, u8 byte_count)
    {
        for (u8 i = 0; i < byte_count; i++)
        {
            dest[i] = (u8)(value & 0xFF);
            value >>= 8;
        }
        return dest + byte_count;
    }

    static u8* stream_write_u32_le(u8* dest, u32 value)
    {
        for (u8 i = 0; i < 4; i++)
        {
            dest[i] = (u8)(value & 0xFF);
            value >>= 8;
        }
        return dest + 4;
    }

    static u8* stream_write_f32_le(u8* dest, f32 value)
    {
        const u32 as_u32 = *((u32*)&value);
        return stream_write_u32_le(dest, as_u32);
    }

    static u8* stream_write_u16_le(u8* dest, u16 value)
    {
        dest[0] = (u8)(value & 0xFF);
        dest[1] = (u8)((value >> 8) & 0xFF);
        return dest + 2;
    }

    static u8* stream_write_data(u8* dest, const u8* data, u32 size)
    {
        memcpy(dest, data, size);
        return dest + size;
    }

    bool stream_write_data(stream_manager_t* m, stream_id_t stream_id, u64 time, const u8* data, u32 size)
    {
        const u32 stream_index = stream_id;

        // Write data to the stream identified by stream_id at the given time
        stream_header_t* stream = m->m_rw_streams[stream_index];
        if (stream->m_write_cursor + (c_relative_time_byte_count + size) <= stream->m_stream_size)
        {
            u8* write_cursor = (u8*)stream + stream->m_write_cursor;
            u64 rtime        = (u64)(time - stream->m_time_begin);
            write_cursor     = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
            write_cursor     = stream_write_data(write_cursor, data, size);
            stream->m_write_cursor += (c_relative_time_byte_count + size);
            stream->m_item_count += 1;
            stream->m_time_end = math::max(time, stream->m_time_end);
            return true;
        }
        return false;  // Not enough space
    }

    bool stream_write_u8(stream_manager_t* m, stream_id_t stream_id, u64 time, u8 value)
    {
        const u32 stream_index = stream_id;

        // Write a u8 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor[0]               = value;
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u8));
        stream->m_item_count += 1;
        stream->m_time_end = math::max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_u16(stream_manager_t* m, stream_id_t stream_id, u64 time, u16 value)
    {
        const u32 stream_index = stream_id;

        // Write a u16 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_u16_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u16));
        stream->m_item_count += 1;
        stream->m_time_end = math::max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_u32(stream_manager_t* m, stream_id_t stream_id, u64 time, u32 value)
    {
        const u32 stream_index = stream_id;

        // Write a u32 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_u32_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u32));
        stream->m_item_count += 1;
        stream->m_time_end = math::max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_f32(stream_manager_t* m, stream_id_t stream_id, u64 time, f32 value)
    {
        const u32 stream_index = stream_id;

        // Write a f32 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_f32_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(f32));
        stream->m_item_count += 1;
        stream->m_time_end = math::max(time, stream->m_time_end);
        return true;
    }

    bool stream_time(stream_manager_t* m, stream_id_t stream_id, u64& out_time_begin, u64& out_time_end)
    {
        const u32 stream_index = stream_id;

        const stream_header_t* header = nullptr;
        if (stream_index < m->m_num_rw_streams)
            header = m->m_rw_streams[stream_index];

        out_time_begin = (header != nullptr) ? header->m_time_begin : 0;
        out_time_end   = (header != nullptr) ? header->m_time_end : 0;

        return (header != nullptr);
    }

    bool stream_info(stream_manager_t* m, stream_id_t stream_id, u64& out_user_id)
    {
        const u32 stream_index = stream_id;

        const stream_header_t* header = nullptr;
        if (stream_index < m->m_num_rw_streams)
            header = m->m_rw_streams[stream_index];

        if (header != nullptr)
        {
            out_user_id = header->m_user_id;
            return true;
        }

        return false;
    }

    i32 stream_read(stream_manager_t* m, stream_id_t stream_id, u64 item_index, u32 item_count, void const*& item_array, u32& item_size)
    {
        // Make sure the stream identified by stream_id is of type c_stream_type_fixed_data or c_stream_type_fixed_u8 or c_stream_type_fixed_u16
        const u32 stream_index = stream_id;

        const stream_header_t* header = nullptr;
        ASSERT(stream_index < (u16)m->m_num_rw_streams);
        header = m->m_rw_streams[stream_index];

        // TODO : implement reading from the stream

        return 0;
    }

    // We want a thread that can create new streams on disk, and we want this to be on a separate
    // thread since we don't want to block the main event loop when creating new streams.
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
        u64                  m_user_id;
        stream_id_t          m_stream_id;
        u64                  m_mmfile_size;
        nmmio::mappedfile_t* m_mmfile;
    };

    struct stream_mapping_t
    {
        u64  m_id;
        char m_name[256 - 8];
    };

    struct stream_thread_t
    {
        alloc_t*          m_allocator;
        const char*       m_base_path;
        channel_t*        m_channel_requests;   // <= channel of stream_request_t*
        channel_t*        m_channel_responses;  // => channel of stream_request_t*
        const char*       m_mappings_filepath;
        struct stat       m_mappings_file_stat;
        stream_mapping_t* m_mappings;
        i32               m_mappings_size;
        i32               m_mappings_capacity;
        stream_request_t* m_active_requests;
        i32               m_active_requests_size;
        i32               m_active_requests_capacity;
    };

    void init_stream_thread(stream_thread_t* thread, alloc_t* allocator, const char* base_path, const char* mappings_filepath)
    {
        thread->m_allocator         = allocator;
        thread->m_base_path         = g_duplicate_string(allocator, base_path);
        thread->m_channel_requests  = channel_init(allocator, 1024);
        thread->m_channel_responses = channel_init(allocator, 1024);
        thread->m_mappings_filepath = g_duplicate_string(allocator, mappings_filepath);
        thread->m_mappings_size     = 0;
        thread->m_mappings_capacity = 1024;
        thread->m_mappings          = g_allocate_array<stream_mapping_t>(allocator, thread->m_mappings_capacity);
        memset(&thread->m_mappings_file_stat, 0, sizeof(struct stat));
        thread->m_active_requests_size     = 0;
        thread->m_active_requests_capacity = 256;
        thread->m_active_requests          = g_allocate_array<stream_request_t>(allocator, thread->m_active_requests_capacity);
    }

    void shutdown_stream_thread(stream_thread_t* thread)
    {
        channel_destroy(thread->m_channel_requests);
        channel_destroy(thread->m_channel_responses);
        g_deallocate_string(thread->m_allocator, thread->m_base_path);
        g_deallocate_string(thread->m_allocator, thread->m_mappings_filepath);
        g_deallocate_array<stream_mapping_t>(thread->m_allocator, thread->m_mappings);
        g_deallocate_array<stream_request_t>(thread->m_allocator, thread->m_active_requests);
    }

    void update_mappings(stream_thread_t* thread)
    {
        // Did the mappings file change on disk?
        FILE* file = fopen(thread->m_mappings_filepath, "rb");
        if (file != nullptr)
        {
            struct stat current_stat;
            if (stat(thread->m_mappings_filepath, &current_stat) == 0)
            {
                if (current_stat.st_mtime != thread->m_mappings_file_stat.st_mtime)
                {
                    // File changed, reload
                    fseek(file, 0, SEEK_END);
                    size_t file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    thread->m_mappings_size = 0;

                    char* file_content = g_allocate_array<char>(thread->m_allocator, file_size);
                    fread(file_content, 1, file_size, file);

                    // Parse the mappings, which are line based and each line is 'ID=filename'
                    ncore::nrunes::reader_t reader(file_content, file_size);
                    while (!reader.end())
                    {
                        crunes_t line = ncore::nrunes::read_line(&reader);
                        crunes_t left, right;
                        if (nrunes::selectLeftAndRightOf(line, '=', left, right))
                        {
                            stream_mapping_t* m = &thread->m_mappings[thread->m_mappings_size++];
                            // left = ID, which is '001122334455' or '00:11:22:33:44:55' format
                            m->m_id = nrunes::parse_mac(left);
                            // right = filename, max 255-8 characters
                            const u32 rlen = math::min((u32)(right.m_end - right.m_str), (u32)DARRAYSIZE(m->m_name) - 1);
                            nmem::memcpy(m->m_name, right.m_ascii + right.m_str, rlen);
                            m->m_name[rlen] = 0;
                        }
                    }
                    thread->m_mappings_file_stat = current_stat;

                    g_deallocate_array<char>(thread->m_allocator, file_content);
                }
            }
            fclose(file);
        }
    }

    stream_mapping_t* pop_known_request(stream_thread_t* thread, stream_request_t*& out_request)
    {
        for (i32 i = 0; i < thread->m_active_requests_size; i++)
        {
            stream_request_t* request = &thread->m_active_requests[i];

            // Check if we have a mapping for this id
            for (i32 j = 0; j < thread->m_mappings_size; j++)
            {
                if (thread->m_mappings[j].m_id == request->m_user_id)
                {
                    // Found a mapping
                    // Remove from active requests
                    thread->m_active_requests[i] = thread->m_active_requests[thread->m_active_requests_size - 1];
                    thread->m_active_requests_size -= 1;
                    out_request = request;
                    return &thread->m_mappings[j];
                }
            }
        }
        out_request = nullptr;
        return nullptr;
    }

    void stream_thread_function(void* arg)
    {
        stream_thread_t* thread = (stream_thread_t*)arg;

        while (true)
        {
            if (thread->m_active_requests_size == 0)
            {
                stream_request_t* request                                   = nullptr;
                request                                                     = (stream_request_t*)channel_pop(thread->m_channel_requests);
                thread->m_active_requests[thread->m_active_requests_size++] = *request;
            }
            else
            {
                stream_request_t* request = nullptr;
                request                   = (stream_request_t*)channel_pop_nowait(thread->m_channel_requests);
                if (request != nullptr)
                {
                    thread->m_active_requests[thread->m_active_requests_size++] = *request;
                }
            }

            update_mappings(thread);

            stream_request_t* known_request;
            stream_mapping_t* known_mapping = pop_known_request(thread, known_request);
            while (known_mapping != nullptr)
            {
                // Create the stream file on disk
                char filepath[MAXPATHLEN];
                snprintf(filepath, sizeof(filepath), "%s/%s.rwstream", thread->m_base_path, known_mapping->m_name);

                // Create the mapped file
                nmmio::mappedfile_t* mmfile_rw = nullptr;
                nmmio::allocate(thread->m_allocator, mmfile_rw);
                if (nmmio::create_rw(mmfile_rw, filepath, known_request->m_mmfile_size))
                {
                    known_request->m_mmfile = mmfile_rw;
                }
                else
                {
                    nmmio::deallocate(thread->m_allocator, mmfile_rw);
                    known_request->m_mmfile = nullptr;
                }

                // Failed or not, push the response back to the main thread
                channel_push(thread->m_channel_responses, (void*)known_request);

                // Check for more known requests
                known_mapping = pop_known_request(thread, known_request);
            }
        }
    }

}  // namespace ncore
