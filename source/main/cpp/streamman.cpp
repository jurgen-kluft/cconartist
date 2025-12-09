#include "ccore/c_allocator.h"
#include "ccore/c_memory.h"
#include "ccore/c_qsort.h"

#include "cconartist/streamman.h"
#include "cmmio/c_mmio.h"

#include <string.h>

namespace ncore
{
    // Note: 4GB is the maximum size of a stream file since we are using 32-bit offsets for items
#define DSTREAM_MAX_FILESIZE ((u64)4 * 1024 * 1024 * 1024)  // 4 GB, since we are using 32-bit offsets for items
#define DSTREAM_NAME_MAXLEN  64

    // Notes:
    // - Memory Mapped Files are not to be used accross different platforms with different endianness
    // - Stream Context is not thread safe, the user must ensure proper locking if multiple threads access the same stream context
    // - Iterators are not thread safe, only one iterator per stream context should be used at a time
    struct stream_header_t
    {
        char m_name[DSTREAM_NAME_MAXLEN];  // Name of the stream
        u64  m_time_begin;                 // Time of the first item in the stream
        u64  m_time_end;                   // Time of the last item in the stream
        u32  m_ids_count;                  // Number of ids currently used
        u32  m_ids_capacity;               // Capacity of the ids array
        u32  m_item_count;                 // Number of items currently in the stream
        u32  m_item_capacity;              // Capacity of items that can be stored in the stream
        u64  m_item_write_cursor;          // Cursor of where the next item will be written in the stream
        // Here in the stream we should be 8 byte aligned
        // Followed by an array of ids (u64) (unsorted)
        // Followed by an array of item offsets (u32)
    };

    // Size of item offset array = DSTREAM_MAX_FILESIZE / (sizeof(u32) +sizeof(stream_item_t) + average_item_data_size)
    // e.g. For sensor data where the average item data size is 4 bytes:
    // DSTREAM_MAX_FILESIZE / (4 + 8 + 4) = 4294967296 / 16 = 256 * 1024 * 1024 = 268435456 items

    // This structure is not part of the memory mapped file, it is used internally
    static s8 s_cmp_ids(const void* lhs, const void* rhs, const void* user_data)
    {
        u16 const  lhs_idx      = *(u16 const*)lhs;
        u16 const  rhs_idx      = *(u16 const*)rhs;
        const u64* unsorted_ids = (const u64*)user_data;
        u64 const  lhs_id       = unsorted_ids[lhs_idx];
        u64 const  rhs_id       = unsorted_ids[rhs_idx];
        if (lhs_id < rhs_id)
            return -1;
        if (lhs_id > rhs_id)
            return 1;
        return 0;
    }

// Stream Item Layout (packed structure)
#pragma pack(push, 1)
    struct stream_item_t
    {
        u64  get_time(u64 stream_begin_time) const;  // Time (6 bytes) is relative to stream start time (unit = ms)
        void set_time(u64 relative_time);            //
        u16  get_id_index() const;                   // Index is indexing into an array of ID's, we can have up to 65536 unique ID's per stream
        void set_id_index(u16 idx);                  //
        // size can be calculated by using the TOC next item offset - this item offset - sizeof(header)
        // followed by data...

        u8 m_time[6];  // 6 bytes time (relative to stream start time), 2 bytes id index
        u8 m_id[2];    // 2 bytes id index
    };

    u64 stream_item_t::get_time(u64 stream_begin_time) const
    {
        u64 time = 0;
        time |= ((u64)m_time[0]) << 0;
        time |= ((u64)m_time[1]) << 8;
        time |= ((u64)m_time[2]) << 16;
        time |= ((u64)m_time[3]) << 24;
        time |= ((u64)m_time[4]) << 32;
        time |= ((u64)m_time[5]) << 40;
        return stream_begin_time + time;
    }

    void stream_item_t::set_time(u64 relative_time)
    {
        m_time[0] = (u8)((relative_time >> 0) & 0xFF);
        m_time[1] = (u8)((relative_time >> 8) & 0xFF);
        m_time[2] = (u8)((relative_time >> 16) & 0xFF);
        m_time[3] = (u8)((relative_time >> 24) & 0xFF);
        m_time[4] = (u8)((relative_time >> 32) & 0xFF);
        m_time[5] = (u8)((relative_time >> 40) & 0xFF);
    }

    u16 stream_item_t::get_id_index() const { return (((u16)m_id[0] << 8) | (u16)m_id[1]); }

    void stream_item_t::set_id_index(u16 idx)
    {
        m_id[0] = (u8)((idx >> 8) & 0xFF);
        m_id[1] = (u8)(idx & 0xFF);
    }

#pragma pack(pop)

    struct stream_context_t;

    struct stream_iterator_t
    {
        i32               m_cur_item_index;
        i32               m_max_item_index;
        stream_context_t* m_stream_context;

        DCORE_CLASS_PLACEMENT_NEW_DELETE
    };

    struct stream_context_t
    {
        alloc_t*             m_allocator;
        nmmio::mappedfile_t* m_mmstream;

        // Stream TOC - read/write
        u8*              m_mmstream_base_rw;
        stream_header_t* m_header;              // Stream header
        u64*             m_ids_array_unsorted;  // The unsorted array of IDs that exists in the stream
        u32*             m_item_offsets;        // Array of item offsets
        u8*              m_items;               // Stream items start here

        // Stream TOC - read-only
        const u8*              m_mmstream_base_ro;
        const stream_header_t* m_header_ro;              // Stream header
        const u64*             m_ids_array_unsorted_ro;  // The unsorted array of IDs that exists in the stream
        const u32*             m_item_offsets_ro;        // Array of item offsets
        const u8*              m_items_ro;               // Stream items start here

        // Stream TOC - shared
        u16* m_ids_idx_array_sorted;  // Array of indices (indirection) into the main ID array, sorted by ID
        int  m_num_active_iterators;

        DCORE_CLASS_PLACEMENT_NEW_DELETE

        i16 find_or_add_id(u64 id);
        i16 find_id(u64 id) const;
    };

    static stream_context_t* new_stream_context(alloc_t* allocator)
    {
        stream_context_t* ctx        = g_construct<stream_context_t>(allocator);
        ctx->m_allocator             = allocator;
        ctx->m_mmstream              = nullptr;
        ctx->m_mmstream_base_rw      = nullptr;
        ctx->m_mmstream_base_ro      = nullptr;
        ctx->m_header                = nullptr;
        ctx->m_ids_array_unsorted    = nullptr;
        ctx->m_item_offsets          = nullptr;
        ctx->m_items                 = nullptr;
        ctx->m_header_ro             = nullptr;
        ctx->m_ids_array_unsorted_ro = nullptr;
        ctx->m_item_offsets_ro       = nullptr;
        ctx->m_items_ro              = nullptr;
        ctx->m_ids_idx_array_sorted  = nullptr;
        ctx->m_num_active_iterators  = 0;
        nmmio::allocate(allocator, ctx->m_mmstream);
        return ctx;
    }

    static void destroy_stream_context(stream_context_t* ctx)
    {
        if (ctx)
        {
            if (ctx->m_ids_idx_array_sorted != nullptr)
            {
                g_deallocate_array(ctx->m_allocator, ctx->m_ids_idx_array_sorted);
                ctx->m_ids_idx_array_sorted = nullptr;
            }
            if (ctx->m_mmstream)
            {
                nmmio::deallocate(ctx->m_allocator, ctx->m_mmstream);
                ctx->m_mmstream         = nullptr;
                ctx->m_mmstream_base_rw = nullptr;
                ctx->m_mmstream_base_ro = nullptr;
            }
            g_destruct(ctx->m_allocator, ctx);
        }
    }

    static void stream_init_toc(stream_context_t* ctx)
    {
        u16* ids_idx_array_sorted = g_allocate_array<u16>(ctx->m_allocator, ctx->m_header_ro->m_ids_capacity);
        for (u32 i = 0; i < ctx->m_header_ro->m_ids_count; i++)
            ids_idx_array_sorted[i] = i;

        ctx->m_ids_idx_array_sorted = ids_idx_array_sorted;

        // Sort the ids array
        nsort::sort(ids_idx_array_sorted, ctx->m_header_ro->m_ids_count, s_cmp_ids, ctx->m_ids_array_unsorted_ro);
    }

    static void stream_init_toc_rw(stream_context_t* ctx, stream_header_t* header, u8* base)
    {
        ctx->m_mmstream_base_rw = base;
        ctx->m_mmstream_base_ro = base;

        ctx->m_header                = header;
        ctx->m_header_ro             = header;
        ctx->m_ids_array_unsorted    = (u64*)(base + sizeof(stream_header_t));
        ctx->m_item_offsets          = (u32*)((u8*)ctx->m_ids_array_unsorted + (sizeof(u64) * ctx->m_header->m_ids_capacity));
        ctx->m_items                 = (u8*)((u8*)ctx->m_item_offsets + (sizeof(u32) * ctx->m_header->m_item_capacity));
        ctx->m_ids_array_unsorted_ro = (const u64*)ctx->m_ids_array_unsorted;
        ctx->m_item_offsets_ro       = (const u32*)ctx->m_item_offsets;
        ctx->m_items_ro              = (const u8*)ctx->m_items;

        stream_init_toc(ctx);
    }

    static void stream_init_toc_ro(stream_context_t* ctx, const stream_header_t* header, const u8* base)
    {
        ctx->m_mmstream_base_ro = base;
        ctx->m_mmstream_base_rw = nullptr;

        ctx->m_header             = nullptr;
        ctx->m_ids_array_unsorted = nullptr;
        ctx->m_item_offsets       = nullptr;
        ctx->m_items              = nullptr;

        ctx->m_header_ro             = header;
        ctx->m_ids_array_unsorted_ro = (const u64*)(base + sizeof(stream_header_t));
        ctx->m_item_offsets_ro       = (const u32*)((const u8*)ctx->m_ids_array_unsorted_ro + (sizeof(u64) * ctx->m_header_ro->m_ids_capacity));
        ctx->m_items_ro              = (const u8*)((const u8*)ctx->m_item_offsets_ro + (sizeof(u32) * ctx->m_header_ro->m_item_capacity));

        stream_init_toc(ctx);
    }

    // Public API
    stream_context_t* stream_open_ro(alloc_t* allocator, const char* abs_filepath, const char* name)
    {
        stream_context_t* ctx = new_stream_context(allocator);
        if (nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (nmmio::open_ro(ctx->m_mmstream, abs_filepath))
            {
                const u8*              base          = (const u8*)nmmio::address_ro(ctx->m_mmstream);
                const stream_header_t* stream_header = (const stream_header_t*)base;
                if (strncmp(stream_header->m_name, name, DSTREAM_NAME_MAXLEN) == 0)
                {
                    stream_init_toc_ro(ctx, stream_header, base);
                    return ctx;
                }
                nmmio::close(ctx->m_mmstream);
            }
        }
        destroy_stream_context(ctx);
        return nullptr;
    }

    stream_context_t* stream_open_rw(alloc_t* allocator, const char* abs_filepath, const char* name)
    {
        stream_context_t* ctx = new_stream_context(allocator);
        if (nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (nmmio::open_rw(ctx->m_mmstream, abs_filepath))
            {
                u8*              base          = (u8*)nmmio::address_rw(ctx->m_mmstream);
                stream_header_t* stream_header = (stream_header_t*)base;
                if (strncmp(stream_header->m_name, name, DSTREAM_NAME_MAXLEN) == 0)
                {
                    stream_init_toc_rw(ctx, stream_header, base);
                    return ctx;
                }
                nmmio::close(ctx->m_mmstream);
            }
        }
        destroy_stream_context(ctx);
        return nullptr;
    }

    stream_context_t* stream_create(alloc_t* allocator, const char* abs_filepath, const char* name, u16 max_ids, u32 average_item_data_size)
    {
        const i32 max_items = (i32)((u32)DSTREAM_MAX_FILESIZE / (u32)(sizeof(u32) + (6 + 2 + average_item_data_size)));

        stream_context_t* ctx = new_stream_context(allocator);

        if (!nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (!nmmio::create_rw(ctx->m_mmstream, abs_filepath, DSTREAM_MAX_FILESIZE))
            {
                destroy_stream_context(ctx);
                return nullptr;
            }
        }
        else
        {
            if (!nmmio::open_rw(ctx->m_mmstream, abs_filepath))
            {
                destroy_stream_context(ctx);
                return nullptr;
            }
        }

        u8*              base          = (u8*)nmmio::address_rw(ctx->m_mmstream);
        stream_header_t* stream_header = (stream_header_t*)base;

        // Initialize the stream
        strncpy(stream_header->m_name, name, DSTREAM_NAME_MAXLEN);
        stream_header->m_name[63]      = '\0';
        stream_header->m_time_begin    = 0;
        stream_header->m_time_end      = 0;
        stream_header->m_ids_count     = 0;
        stream_header->m_ids_capacity  = max_ids;
        stream_header->m_item_count    = 0;
        stream_header->m_item_capacity = max_items;

        stream_init_toc_rw(ctx, stream_header, base);

        // Initialize write offset
        stream_header->m_item_write_cursor = sizeof(stream_header_t) + (sizeof(u64) * stream_header->m_ids_capacity) + (sizeof(u32) * stream_header->m_item_capacity);

        nmmio::sync(ctx->m_mmstream);
        return ctx;
    }

    bool stream_close(stream_context_t*& ctx)
    {
        if (ctx != nullptr)
        {
            if (ctx->m_mmstream != nullptr)
            {
                nmmio::sync(ctx->m_mmstream);
                nmmio::close(ctx->m_mmstream);
            }
            destroy_stream_context(ctx);
            ctx = nullptr;
            return true;
        }
        return false;
    }

    void stream_sync(stream_context_t* ctx)
    {
        if (ctx != nullptr && ctx->m_mmstream != nullptr)
            nmmio::sync(ctx->m_mmstream);
    }

    void stream_destroy(stream_context_t* ctx)
    {
        if (ctx)
        {
            ASSERT(ctx->m_num_active_iterators == 0);
            destroy_stream_context(ctx);
        }
    }

    // Create a stream iterator
    stream_iterator_t* stream_create_iterator(stream_context_t* ctx)
    {
        ctx->m_num_active_iterators++;
        stream_iterator_t* iterator = g_construct<stream_iterator_t>(ctx->m_allocator);
        iterator->m_cur_item_index  = 0;
        iterator->m_stream_context  = ctx;
        // Determine the maximum valid item index (last index). If there are no items, set to -1.
        {
            const u32 item_count = (ctx->m_header != nullptr) ? ctx->m_header->m_item_count : ctx->m_header_ro->m_item_count;
            if (item_count == 0)
            {
                iterator->m_max_item_index = -1;
            }
            else
            {
                iterator->m_max_item_index = (i32)(item_count - 1);
            }
        }
        return iterator;
    }

    void stream_destroy_iterator(stream_context_t* ctx, stream_iterator_t* iterator)
    {
        if (iterator != nullptr && iterator->m_stream_context == ctx)
        {
            ctx->m_num_active_iterators--;
            g_destruct(ctx->m_allocator, iterator);
        }
    }

    // Move iterator forward or backward by items_to_advance
    void stream_iterator_advance(stream_iterator_t* iterator, i64 items_to_advance)
    {
        if (iterator->m_max_item_index < 0)
            return;  // No items in the stream

        i64 new_index = (i64)iterator->m_cur_item_index + items_to_advance;
        if (new_index < 0)
            new_index = 0;
        if (new_index > (i64)iterator->m_max_item_index)
            new_index = (i64)iterator->m_max_item_index;
        iterator->m_cur_item_index = (i32)new_index;
    }

    bool stream_is_full(stream_context_t* ctx)
    {
        if (ctx->m_header != nullptr)
        {
            const stream_header_t* hdr = ctx->m_header;
            return (hdr->m_item_write_cursor >= (u64)DSTREAM_MAX_FILESIZE);
        }
        else
        {
            const stream_header_t* hdr = ctx->m_header_ro;
            return (hdr->m_item_write_cursor >= (u64)DSTREAM_MAX_FILESIZE);
        }
    }

    // Write an item to the stream
    bool stream_write_item(stream_context_t* ctx, u64 time, u64 id, const void* data, u32 size)
    {
        if (ctx->m_header == nullptr)
            return false;

        stream_header_t* hdr = ctx->m_header;

        // Check if stream is full, cannot write more data
        if (hdr->m_item_write_cursor + (u64)sizeof(stream_item_t) + (u64)size > DSTREAM_MAX_FILESIZE)
        {
            hdr->m_item_write_cursor = DSTREAM_MAX_FILESIZE;
            return false;
        }

        // Check item capacity
        if (hdr->m_item_count >= hdr->m_item_capacity)
        {
            hdr->m_item_write_cursor = DSTREAM_MAX_FILESIZE;
            return false;
        }

        u8*            base          = ctx->m_mmstream_base_rw;
        const u32      item_offset   = (u32)(hdr->m_item_write_cursor);
        stream_item_t* item          = (stream_item_t*)(base + item_offset);
        u8*            item_data_ptr = (u8*)(base + item_offset + sizeof(stream_item_t));

        if (hdr->m_item_count == 0)
        {
            // First item, set stream start time
            hdr->m_time_begin = time;
        }

        // Find or add ID
        i16 id_index = ctx->find_or_add_id(id);
        if (id_index < 0)
            return false;
        ASSERT(id_index >= 0 && (u16)id_index < hdr->m_ids_count);

        // Set item header
        item->set_time(time - hdr->m_time_begin);
        item->set_id_index((i16)id_index);

        // Copy item data
        memcpy(item_data_ptr, data, size);

        // Update item offset array
        ctx->m_item_offsets[hdr->m_item_count] = item_offset;

        // Update header
        hdr->m_item_count += 1;
        hdr->m_item_write_cursor += (sizeof(stream_item_t) + size);

        // Update stream end time
        if (time > hdr->m_time_end)
            hdr->m_time_end = time;

        return true;
    }

    // Get item, item index is relative to iterator, this is technically a read-only operation.
    bool stream_iterator_get_item(stream_iterator_t* iterator, u64 relative_item_index, u64& out_item_time, u64& out_item_id, const u8*& out_item_data, u32& out_item_size)
    {
        if (iterator->m_max_item_index >= 0)
        {
            const u64 absolute_item_index = (u64)iterator->m_cur_item_index + relative_item_index;

            const stream_context_t* ctx = iterator->m_stream_context;
            const stream_header_t*  hdr = (ctx->m_header_ro != nullptr) ? ctx->m_header_ro : ctx->m_header;
            if (hdr != nullptr && absolute_item_index < (u64)hdr->m_item_count)
            {
                const u32            item_offset = ctx->m_item_offsets_ro[absolute_item_index];
                const u8*            item_ptr    = ctx->m_mmstream_base_ro + item_offset;
                const u8*            item_data   = item_ptr + sizeof(stream_item_t);
                const stream_item_t* header      = (stream_item_t*)item_ptr;

                out_item_time      = header->get_time(hdr->m_time_begin);
                const u16 id_index = header->get_id_index();
                out_item_id        = ctx->m_ids_array_unsorted_ro[id_index];

                if ((absolute_item_index + 1) < (u64)hdr->m_item_count)
                {
                    const u32 next_item_offset = ctx->m_item_offsets_ro[absolute_item_index + 1];
                    out_item_size              = (next_item_offset - item_offset) - sizeof(stream_item_t);
                }
                else
                {
                    out_item_size = (hdr->m_item_write_cursor - item_offset) - sizeof(stream_item_t);
                }

                out_item_data = item_data;
                return true;
            }
        }

        out_item_time = 0;
        out_item_id   = 0;
        out_item_data = nullptr;
        out_item_size = 0;
        return false;
    }

}  // namespace ncore
