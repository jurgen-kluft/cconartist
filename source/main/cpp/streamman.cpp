#include "ccore/c_allocator.h"
#include "ccore/c_math.h"
#include "ccore/c_memory.h"

#include "cconartist/streamman.h"

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

    struct stream_id_info_t
    {
        estream_type::enum_t m_stream_type;
        estream_mode::enum_t m_stream_mode;
        u16                  m_stream_index;
    };

    static stream_id_info_t s_decode_stream_id(stream_id_t stream_id)
    {
        stream_id_info_t info;
        info.m_stream_type  = (estream_type::enum_t)((stream_id >> 24) & 0xFF);
        info.m_stream_mode  = (estream_mode::enum_t)((stream_id >> 16) & 0xFF);
        info.m_stream_index = (u16)(stream_id & 0xFFFF);
        return info;
    }

    static stream_id_t s_encode_stream_id(stream_id_info_t const& info) { return ((stream_id_t)info.m_stream_type << 24) | ((stream_id_t)info.m_stream_mode << 16) | (stream_id_t)info.m_stream_index; }

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
        i32                     m_num_ro_streams;
        i32                     m_max_ro_streams;
        nmmio::mappedfile_t**   m_ro_stream_files;
        const stream_header_t** m_ro_streams;
        const stream_header_t** m_ro_streams_sorted;  // For quick lookup by user_id
        i32                     m_num_rw_streams;
        i32                     m_max_rw_streams;
        char**                  m_rw_stream_filepaths;
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
        for (i32 i = 0; i < manager->m_num_rw_streams; i++)
        {
            nmmio::mappedfile_t* rw_file = manager->m_rw_stream_files[i];
            if (rw_file != nullptr)
            {
                nmmio::sync(rw_file);
            }
        }
    }

    void stream_manager_update(stream_manager_t* manager)
    {
        // Check time since last update and skip if too soon
        //  - We only want to do this once in a while, e.g. once every day
        // Update the stream manager
        //  - Check if any streams need to 'grow'
        //    We can do this by 'closing' the stream, and opening it again with a larger size.
        // Using m_time_begin and m_time_end together with m_item_count we can determine the throughput
        // and determine how many days a stream still has to go before it should be extended in size.
    }

    void stream_manager_destroy(alloc_t* allocator, stream_manager_t*& manager)
    {
        // Destroy the stream manager
        //  - Flush and close all open streams
        //  - Deallocate all memory used by the stream manager

        // Close all read-write streams
        for (i32 i = 0; i < manager->m_num_rw_streams; i++)
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
    stream_id_t stream_register(stream_manager_t* m, estream_type::enum_t stream_type, const char* name, u64 user_id, u64 file_size, u32 sizeof_item)
    {
        // Create a new read-write stream and return its stream_id
        // Initialize m_time_begin and m_time_end to the current time
        stream_id_info_t info;
        info.m_stream_type  = stream_type;
        info.m_stream_mode  = estream_mode::readwrite;
        info.m_stream_index = (u16)m->m_num_rw_streams;

        // Make sure there is enough space for the new stream
        stream_manager_resize_rw(m);

        const u16 user_index = stream_manager_largest_user_index_for(m, user_id);

        nmmio::mappedfile_t* mmfile_rw = nullptr;
        nmmio::allocate(m->m_allocator, mmfile_rw);
        char filepath[MAXPATHLEN];
        snprintf(filepath, sizeof(filepath), "%s/%s.rwstream", m->m_base_path, name);
        if (nmmio::create_rw(mmfile_rw, filepath, file_size))
        {
            // Initialize the stream header
            stream_header_t* header = (stream_header_t*)nmmio::address_rw(mmfile_rw);
            header->m_user_id       = user_id;
            header->m_stream_type   = (u16)stream_type;
            header->m_sizeof_item   = sizeof_item;
            header->m_user_index    = user_index + 1;
            header->m_reserved1     = 0;
            header->m_reserved2     = 0;
            header->m_time_begin    = (u64)time(nullptr);
            header->m_stream_size   = file_size;
            header->m_item_count    = 0;
            header->m_time_end      = header->m_time_begin;
            header->m_write_cursor  = sizeof(stream_header_t);

            // Register the read-write stream
            m->m_rw_stream_filepaths[m->m_num_rw_streams] = g_allocate_array<char>(m->m_allocator, strlen(filepath) + 1);
            strlcpy((char*)m->m_rw_stream_filepaths[m->m_num_rw_streams], filepath, strlen(filepath) + 1);
            m->m_rw_stream_files[m->m_num_rw_streams] = mmfile_rw;
            m->m_rw_streams[m->m_num_rw_streams]      = header;
            m->m_num_rw_streams += 1;
        }
        else
        {
            nmmio::deallocate(m->m_allocator, mmfile_rw);
        }

        return s_encode_stream_id(info);
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
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite);
        ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != estream_type::TypeFixed || size > m->m_rw_streams[info.m_stream_index]->m_sizeof_item)
        {
            return false;
        }

        // Write data to the stream identified by stream_id at the given time
        stream_header_t* stream = m->m_rw_streams[info.m_stream_index];
        if (stream->m_write_cursor + (c_relative_time_byte_count + size) <= stream->m_stream_size)
        {
            u8* write_cursor = (u8*)stream + stream->m_write_cursor;
            u64 rtime        = (u64)(time - stream->m_time_begin);
            write_cursor     = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
            write_cursor     = stream_write_data(write_cursor, data, size);
            stream->m_write_cursor += (c_relative_time_byte_count + size);
            stream->m_item_count += 1;
            stream->m_time_end = math::g_max(time, stream->m_time_end);
            return true;
        }
        return false;  // Not enough space
    }

    bool stream_write_u8(stream_manager_t* m, stream_id_t stream_id, u64 time, u8 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite);
        ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != estream_type::TypeU8)
        {
            return false;
        }

        // Write a u8 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[info.m_stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor[0]               = value;
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u8));
        stream->m_item_count += 1;
        stream->m_time_end = math::g_max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_u16(stream_manager_t* m, stream_id_t stream_id, u64 time, u16 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite);
        ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != estream_type::TypeU16)
        {
            return false;
        }

        // Write a u16 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[info.m_stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_u16_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u16));
        stream->m_item_count += 1;
        stream->m_time_end = math::g_max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_u32(stream_manager_t* m, stream_id_t stream_id, u64 time, u32 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite);
        ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != estream_type::TypeU32)
        {
            return false;
        }

        // Write a u32 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[info.m_stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_u32_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(u32));
        stream->m_item_count += 1;
        stream->m_time_end = math::g_max(time, stream->m_time_end);
        return true;
    }

    bool stream_write_f32(stream_manager_t* m, stream_id_t stream_id, u64 time, f32 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite);
        ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != estream_type::TypeF32)
        {
            return false;
        }

        // Write a f32 value to the stream identified by stream_id at the given time
        stream_header_t* stream       = m->m_rw_streams[info.m_stream_index];
        u8*              write_cursor = (u8*)stream + stream->m_write_cursor;
        u64              rtime        = (u64)(time - stream->m_time_begin);
        write_cursor                  = stream_write_u64_le(write_cursor, rtime, c_relative_time_byte_count);
        write_cursor                  = stream_write_f32_le(write_cursor, value);
        stream->m_write_cursor += (c_relative_time_byte_count + sizeof(f32));
        stream->m_item_count += 1;
        stream->m_time_end = math::g_max(time, stream->m_time_end);
        return true;
    }

    bool stream_time(stream_manager_t* m, stream_id_t stream_id, u64& out_time_begin, u64& out_time_end)
    {
        const stream_id_info_t info   = s_decode_stream_id(stream_id);
        const stream_header_t* header = nullptr;
        if (info.m_stream_mode == estream_mode::readwrite)
        {
            if (info.m_stream_index < m->m_num_rw_streams)
                header = m->m_rw_streams[info.m_stream_index];
        }
        else if (info.m_stream_mode == estream_mode::readonly)
        {
            if (info.m_stream_index < m->m_num_ro_streams)
                header = m->m_ro_streams[info.m_stream_index];
        }

        out_time_begin = (header != nullptr) ? header->m_time_begin : 0;
        out_time_end   = (header != nullptr) ? header->m_time_end : 0;

        return (header != nullptr);
    }

    bool stream_info(stream_manager_t* m, stream_id_t stream_id, u64& out_user_id)
    {
        const stream_id_info_t info   = s_decode_stream_id(stream_id);
        const stream_header_t* header = nullptr;
        if (info.m_stream_mode == estream_mode::readwrite)
        {
            if (info.m_stream_index < m->m_num_rw_streams)
                header = m->m_rw_streams[info.m_stream_index];
        }
        else if (info.m_stream_mode == estream_mode::readonly)
        {
            if (info.m_stream_index < m->m_num_ro_streams)
                header = m->m_ro_streams[info.m_stream_index];
        }

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
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        ASSERT(info.m_stream_mode == estream_mode::readwrite || info.m_stream_mode == estream_mode::readonly);

        const stream_header_t* header = nullptr;
        if (info.m_stream_mode == estream_mode::readwrite)
        {
            ASSERT(info.m_stream_index < (u16)m->m_num_rw_streams);
            header = m->m_rw_streams[info.m_stream_index];
        }
        else if (info.m_stream_mode == estream_mode::readonly)
        {
            ASSERT(info.m_stream_index < (u16)m->m_num_ro_streams);
            header = m->m_ro_streams[info.m_stream_index];
        }
        else
        {
            ASSERT(false);  // Invalid stream mode
            return 0;
        }

        // TODO : implement reading from the stream

        return 0;
    }

}  // namespace ncore
