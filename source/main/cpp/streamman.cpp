#include "ccore/c_allocator.h"
#include "ccore/c_memory.h"
#include "ccore/c_qsort.h"
#include "ccore/c_runes.h"

#include "cconartist/streamman.h"

#include "clibuv/uv.h"
#include "cmmio/c_mmio.h"

#include <time.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace ncore
{
    // Notes:
    // - Stream Files are not to be used accross different platforms with different endianness
    // - All is not thread safe, the user must ensure proper locking if multiple threads access this
    // - stream_manager_t::update ?, where every night it analyzes the streams and finalize/rotates them if needed.
    // - file naming convention: {name}_XXXX.stream, where XXXX is a 4 digit incrementing number.

    typedef s8          stream_mode_t;
    const stream_mode_t c_stream_mode_readonly  = 1;
    const stream_mode_t c_stream_mode_readwrite = 2;

    struct stream_id_info_t
    {
        stream_type_t m_stream_type;
        stream_mode_t m_stream_mode;
        u16           m_stream_index;
    };

    static stream_id_info_t s_decode_stream_id(stream_id_t stream_id)
    {
        stream_id_info_t info;
        info.m_stream_type  = (stream_type_t)((stream_id >> 24) & 0xFF);
        info.m_stream_mode  = (stream_mode_t)((stream_id >> 16) & 0xFF);
        info.m_stream_index = (u16)(stream_id & 0xFFFF);
        return info;
    }

    static stream_id_t s_encode_stream_id(stream_id_info_t const& info) { return ((stream_id_t)info.m_stream_type << 24) | ((stream_id_t)info.m_stream_mode << 16) | (stream_id_t)info.m_stream_index; }

    struct stream_header_t
    {
        u64 m_user_id;       // Stream identifier
        u16 m_user_type;     // Type of the stream
        u16 m_stream_type;   // (stream_type_t) Type of stream
        u32 m_sizeof_item;   // Size of each item in the stream (for fixed size streams)
        u64 m_reserved2;     // Reserved for future use
        u64 m_time_begin;    // Time of the first item in the stream
        u64 m_stream_size;   // Size of the stream file
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
        const stream_header_t** m_ro_streams_sorted;  // For quick lookup by user_id/user_type
        i32                     m_num_rw_streams;
        i32                     m_max_rw_streams;
        const char**            m_rw_stream_filepaths;
        nmmio::mappedfile_t**   m_rw_stream_files;
        stream_header_t**       m_rw_streams;
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
            const char**          new_rw_stream_filepaths = g_reallocate_array<const char*>(m->m_allocator, m->m_rw_stream_filepaths, m->m_max_rw_streams, new_max_rw_streams);
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
                m->m_num_ro_streams += 1;
            }
            else
            {
                nmmio::close(mmfile_ro);
            }
        }
    }

    i32 stream_manager_count_ro_streams(stream_manager_t* m, u64 user_id, u16 user_type)
    {
        i32 count = 0;
        for (i32 i = 0; i < m->m_num_ro_streams; i++)
        {
            const stream_header_t* header = m->m_ro_streams[i];
            if ((header->m_user_id == user_id) && (header->m_user_type == user_type))
            {
                count += 1;
            }
        }
        return count;
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
                strcpy((char*)m->m_rw_stream_filepaths[m->m_num_rw_streams], filepath);
                m->m_rw_stream_files[m->m_num_rw_streams] = mmfile_rw;
                m->m_rw_streams[m->m_num_rw_streams]      = header;
                m->m_num_rw_streams += 1;
            }
            else
            {
                nmmio::close(mmfile_rw);
            }
        }
    }

    stream_manager_t* stream_manager_create(alloc_t* allocator, i32 max_streams, const char* base_path)
    {
        // Create the stream manager
        //   - Create arrays for read-only and read-write streams

        stream_manager_t* m = g_construct<stream_manager_t>(allocator);
        m->m_base_path      = g_allocate_array<char>(allocator, strlen(base_path) + 1);
        strcpy((char*)m->m_base_path, base_path);
        m->m_last_update_time    = time(nullptr);  // Current time
        m->m_allocator           = allocator;
        m->m_num_ro_streams      = 0;
        m->m_max_ro_streams      = max_streams;
        m->m_ro_streams          = g_allocate_array<const stream_header_t*>(allocator, max_streams);
        m->m_ro_streams_sorted   = g_allocate_array<const stream_header_t*>(allocator, max_streams);
        m->m_num_rw_streams      = 0;
        m->m_max_rw_streams      = max_streams;
        m->m_rw_stream_filepaths = g_allocate_array<const char*>(allocator, max_streams);
        m->m_rw_stream_files     = g_allocate_array<nmmio::mappedfile_t*>(allocator, max_streams);
        m->m_rw_streams          = g_allocate_array<stream_header_t*>(allocator, max_streams);

        char full_path[MAXPATHLEN];

        // Scan the base path for existing stream files:
        //   - *.ro.stream -> read-only streams
        //   - *.rw.stream -> read-write streams
        DIR* dir = opendir(base_path);
        if (dir)
        {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (has_ro_extension(entry->d_name))
                {
                    snprintf(full_path, sizeof(full_path), "%s/%s", m->m_base_path, entry->d_name);
                    stream_manager_add_ro_stream(m, full_path);
                }
                else if (has_rw_extension(entry->d_name))
                {
                    snprintf(full_path, sizeof(full_path), "%s/%s", m->m_base_path, entry->d_name);
                    stream_manager_add_rw_stream(m, full_path);
                }
            }
            closedir(dir);
        }
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
            }
        }
        g_deallocate_array<const char*>(allocator, manager->m_rw_stream_filepaths);
        g_deallocate_array<nmmio::mappedfile_t*>(allocator, manager->m_rw_stream_files);
        g_deallocate_array<stream_header_t*>(allocator, manager->m_rw_streams);

        // Close all read-only streams
        for (i32 i = 0; i < manager->m_num_ro_streams; i++)
        {
            nmmio::mappedfile_t* ro_file = (nmmio::mappedfile_t*)manager->m_ro_streams[i];
            if (ro_file != nullptr)
            {
                nmmio::close(ro_file);
                nmmio::deallocate(manager->m_allocator, ro_file);
            }
        }

        g_deallocate_array<nmmio::mappedfile_t*>(allocator, manager->m_ro_stream_files);
        g_deallocate_array<const stream_header_t*>(allocator, manager->m_ro_streams);
        g_deallocate_array<const stream_header_t*>(allocator, manager->m_ro_streams_sorted);

        g_deallocate_array<char>(allocator, manager->m_base_path);

        g_destruct(allocator, manager);
    }

    // Required: filename and extension need to be ASCII and are case-insensitive
    static bool s_has_filename(const char* filepath, const char* filename)
    {
        i32 filepath_len = (i32)strnlen(filepath, MAXPATHLEN);

        // Walk backwards to find the filename part, first the '.' to skip the extension
        const char* fp  = filepath + filepath_len - 1;
        const char* end = filepath + filepath_len;
        while (*fp != '.')
        {
            if (fp == filepath)
                return false;  // No '.' found, invalid
            fp--;
        }
        // Now skip the '-00', which is the file index
        fp -= 2;
        if (*fp != '-')
            return false;  // Invalid format
        fp -= 1;

        // Now walk backwards to find the '/' or start of string to get the base filename
        const char* fp_end = fp;
        while (*fp != '/' && *fp != '\\')
        {
            if (fp == filepath)
                break;  // Reached the start of the string
            fp--;
        }

        // Now compare the filename part (case insensitive)
        const char* fn = filename;
        while (*fn != '\0' && *fp != '\0' && fp < fp_end)
        {
            if (ascii::to_lower(*fn) != ascii::to_lower(*fp))
                return false;
            fn++;
            fp++;
        }
        return (*fn == '\0') && (*fp == '.');
    }

    stream_id_t stream_register(stream_manager_t* m, stream_type_t stream_type, const char* name, u64 user_id, u16 user_type, u64 file_size, u32 sizeof_item)
    {
        // Check if a current read-write stream with this name exists, if so return its stream_id
        for (i32 i = 0; i < m->m_num_rw_streams; i++)
        {
            if (s_has_filename(m->m_rw_stream_filepaths[i], name))
            {
                stream_id_info_t info;
                info.m_stream_type  = stream_type;
                info.m_stream_mode  = c_stream_mode_readwrite;
                info.m_stream_index = (u16)i;
                return s_encode_stream_id(info);
            }
        }
        // Otherwise create a new read-write stream and return its stream_id
        // Also initialize m_time_begin and m_time_end to the current time
        stream_id_info_t info;
        info.m_stream_type  = stream_type;
        info.m_stream_mode  = c_stream_mode_readwrite;
        info.m_stream_index = (u16)m->m_num_rw_streams;

        stream_manager_resize_rw(m);

        const i32 stream_index = stream_manager_count_ro_streams(m, user_id, user_type);

        nmmio::mappedfile_t* mmfile_rw = nullptr;
        nmmio::allocate(m->m_allocator, mmfile_rw);
        char filepath[MAXPATHLEN];
        snprintf(filepath, sizeof(filepath), "%s/%s-%02i.rwstream", m->m_base_path, name, stream_index);
        if (nmmio::create_rw(mmfile_rw, filepath, file_size))
        {
            // Initialize the stream header
            stream_header_t* header = (stream_header_t*)nmmio::address_rw(mmfile_rw);
            header->m_user_id       = user_id;
            header->m_user_type     = user_type;
            header->m_stream_type   = (u16)stream_type;
            header->m_sizeof_item   = sizeof_item;
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

    static u8* stream_write_u64_le(u8* dest, u64 value, s8 byte_count)
    {
        for (s8 i = 0; i < byte_count; i++)
        {
            dest[i] = (u8)(value & 0xFF);
            value >>= 8;
        }
        return dest + byte_count;
    }

    static u8* stream_write_u32_le(u8* dest, u32 value, s8 byte_count)
    {
        for (s8 i = 0; i < byte_count; i++)
        {
            dest[i] = (u8)(value & 0xFF);
            value >>= 8;
        }
        return dest + byte_count;
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
        // Make sure the stream identified by stream_id is of type c_stream_type_fixed_data
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != c_stream_type_fixed_data || size > m->m_rw_streams[info.m_stream_index]->m_sizeof_item)
        {
            return false;
        }

        // Write data to the stream identified by stream_id at the given time
        u8* stream_ptr = (u8*)m->m_rw_streams[info.m_stream_index] + m->m_rw_streams[info.m_stream_index]->m_write_cursor;
        u64 rtime      = (u64)(time - m->m_rw_streams[info.m_stream_index]->m_time_begin);
        stream_ptr     = stream_write_u64_le(stream_ptr, rtime, 5);
        stream_ptr     = stream_write_data(stream_ptr, data, size);
        m->m_rw_streams[info.m_stream_index]->m_write_cursor += (5 + size);
        m->m_rw_streams[info.m_stream_index]->m_item_count += 1;
        if (time > m->m_rw_streams[info.m_stream_index]->m_time_end)
        {
            m->m_rw_streams[info.m_stream_index]->m_time_end = time;
        }
        return true;
    }

    bool stream_write_u8(stream_manager_t* m, stream_id_t stream_id, u64 time, u8 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        // Make sure the stream identified by stream_id is of type c_stream_type_fixed_u8
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != c_stream_type_fixed_u8)
        {
            return false;
        }

        // Write a u8 value to the stream identified by stream_id at the given time
        u8* stream_ptr = (u8*)m->m_rw_streams[info.m_stream_index] + m->m_rw_streams[info.m_stream_index]->m_write_cursor;
        u64 rtime      = (u64)(time - m->m_rw_streams[info.m_stream_index]->m_time_begin);
        stream_ptr     = stream_write_u64_le(stream_ptr, rtime, 5);
        stream_ptr[0]  = value;
        m->m_rw_streams[info.m_stream_index]->m_write_cursor += (5 + 1);
        m->m_rw_streams[info.m_stream_index]->m_item_count += 1;
        if (time > m->m_rw_streams[info.m_stream_index]->m_time_end)
        {
            m->m_rw_streams[info.m_stream_index]->m_time_end = time;
        }
        return true;
    }

    bool stream_write_u16(stream_manager_t* m, stream_id_t stream_id, u64 time, u16 value)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);

        // Make sure the stream identified by stream_id is of type c_stream_type_fixed_u16
        if (m->m_rw_streams[info.m_stream_index]->m_stream_type != c_stream_type_fixed_u16)
        {
            return false;
        }

        // Write a u16 value to the stream identified by stream_id at the given time
        u8* stream_ptr = (u8*)m->m_rw_streams[info.m_stream_index] + m->m_rw_streams[info.m_stream_index]->m_write_cursor;
        u64 rtime      = (u64)(time - m->m_rw_streams[info.m_stream_index]->m_time_begin);
        stream_ptr     = stream_write_u64_le(stream_ptr, rtime, 5);
        stream_ptr     = stream_write_u16_le(stream_ptr, value);
        m->m_rw_streams[info.m_stream_index]->m_write_cursor += (5 + 2);
        m->m_rw_streams[info.m_stream_index]->m_item_count += 1;
        if (time > m->m_rw_streams[info.m_stream_index]->m_time_end)
        {
            m->m_rw_streams[info.m_stream_index]->m_time_end = time;
        }
        return true;
    }

    bool stream_time_range(stream_manager_t* m, stream_id_t stream_id, u64& out_time_begin, u64& out_time_end)
    {
        const stream_id_info_t info = s_decode_stream_id(stream_id);
        if (info.m_stream_index >= m->m_num_ro_streams)
            return false;
        const stream_header_t* header = nullptr;
        if (info.m_stream_mode == c_stream_mode_readwrite)
        {
            header = m->m_rw_streams[info.m_stream_index];
        }
        else if (info.m_stream_mode == c_stream_mode_readonly)
        {
            header = m->m_ro_streams[info.m_stream_index];
        }
        if (header != nullptr)
        {
            out_time_begin = header->m_time_begin;
            out_time_end   = header->m_time_end;
        }
        return (header != nullptr);
    }

    i32 stream_read(stream_manager_t* m, stream_id_t stream_id, u64 item_index, u32 item_count, void const*& item_array, u32& item_size)
    {
        // Make sure the stream identified by stream_id is of type c_stream_type_fixed_data or c_stream_type_fixed_u8 or c_stream_type_fixed_u16
        const stream_id_info_t info = s_decode_stream_id(stream_id);

        const stream_header_t* header = nullptr;
        if (info.m_stream_mode == c_stream_mode_readwrite)
        {
            header = m->m_rw_streams[info.m_stream_index];
        }
        else if (info.m_stream_mode == c_stream_mode_readonly)
        {
            header = m->m_ro_streams[info.m_stream_index];
        }
        else
        {
            return 0;
        }



        return 0;
    }

}  // namespace ncore
