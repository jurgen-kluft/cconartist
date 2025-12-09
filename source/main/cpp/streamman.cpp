#include "ccore/c_allocator.h"
#include "ccore/c_memory.h"
#include "ccore/c_qsort.h"
#include "cconartist/streamman.h"

#include <string.h>

namespace ncore
{
    // Every item is alligned to 4 bytes, so with 32 bits we can address up to 16GB of stream data.
    // Note: 16GB is the maximum size of a stream file.
    struct stream_header_t
    {
        char m_name[64];           // Name of the stream
        u64  m_time_begin;         // Time of the first item in the stream
        u64  m_time_end;           // Time of the last item in the stream
        i32  m_ids_count;          // Capacity of pages allocated
        u32  m_ids_capacity;       // Capacity of pages allocated
        u32  m_item_count;         // Number of items currently in the stream
        u32  m_item_capacity;      // Capacity of items that can be stored in the stream
        u64  m_item_write_offset;  // Offset where the next item will be written
        // Followed by an array of ids (u64) (unsorted)
        // Followed by an array of item offsets (u32)
    };

#define DSTREAM_MAX_FILESIZE ((u64)4 * 1024 * 1024 * 1024)  // 4 GB, since we are using 32-bit offsets for items

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

    // Stream Item Layout
    struct stream_item_t
    {
        u64  get_time(u64 stream_begin_time) const;  // Time (6 bytes) is relative to stream start time (unit = ms)
        void set_time(u64 relative_time);            //
        i16  get_id_index() const;                   // Index is indexing into an array of ID's, we can have up to 65536 unique ID's per stream
        void set_id_index(i16 idx);                  //
        // size can be calculated by using the TOC next item offset - this item offset - sizeof(header)
        // followed by data...

        u8 m_time[6];  // 6 bytes time (relative to stream start time), 2 bytes id index
        u8 m_id[2];    // 2 bytes id index
    };

    struct stream_context_t;

    struct stream_iterator_t
    {
        i32               m_cur_item_index;
        i32               m_max_item_index;
        stream_context_t* m_stream_context;
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

        int m_num_active_iterators;

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
                g_destruct(ctx->m_allocator, ctx->m_ids_idx_array_sorted);
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

    static void stream_init_toc_rw(stream_context_t*& ctx, stream_header_t* header, u8* base)
    {
        // u8* ids_array_ptr     = base + sizeof(stream_header_t);
        // u8* items_offsets_ptr = ids_array_ptr + (sizeof(u64) * ctx->m_header->m_ids_capacity);
        // u8* items_data_ptr    = items_offsets_ptr + (sizeof(u32) * ctx->m_header->m_item_capacity);

        // ctx->m_ids_array_unsorted = (u64*)ids_array_ptr;      // IDs array starts right after the header
        // ctx->m_item_offsets       = (u32*)items_offsets_ptr;  // Item offsets start after the IDs array
        // ctx->m_items              = (u8*)items_data_ptr;      // Items data starts after the item offsets array

        ctx->m_header                = header;
        ctx->m_header_ro             = header;
        ctx->m_ids_array_unsorted    = (u64*)(base + sizeof(stream_header_t));
        ctx->m_item_offsets          = (u32*)(base + sizeof(stream_header_t) + (sizeof(u64) * ctx->m_header->m_ids_capacity));
        ctx->m_items                 = (u8*)(base + sizeof(stream_header_t) + (sizeof(u64) * ctx->m_header->m_ids_capacity) + (sizeof(u32) * ctx->m_header->m_item_capacity));
        ctx->m_ids_array_unsorted_ro = (const u64*)ctx->m_ids_array_unsorted;
        ctx->m_item_offsets_ro       = (const u32*)ctx->m_item_offsets;
        ctx->m_items_ro              = (const u8*)ctx->m_items;

        u16* ids_idx_array_sorted = g_allocate_array<u16>(ctx->m_allocator, ctx->m_header->m_ids_capacity);
        for (i32 i = 0; i < ctx->m_header->m_ids_count; i++)
            ids_idx_array_sorted[i] = i;

        ctx->m_ids_idx_array_sorted = ids_idx_array_sorted;

        // Sort the ids array
        nsort::sort(ids_idx_array_sorted, ctx->m_header->m_ids_count, s_cmp_ids, ctx->m_ids_array_unsorted);
    }

    static void stream_init_toc_ro(stream_context_t*& ctx, const stream_header_t* header, const u8* base)
    {
        ctx->m_header             = nullptr;
        ctx->m_ids_array_unsorted = nullptr;
        ctx->m_item_offsets       = nullptr;
        ctx->m_items              = nullptr;

        ctx->m_header_ro             = header;
        ctx->m_ids_array_unsorted_ro = (const u64*)(base + sizeof(stream_header_t));
        ctx->m_item_offsets_ro       = (const u32*)(base + sizeof(stream_header_t) + (sizeof(u64) * ctx->m_header_ro->m_ids_capacity));
        ctx->m_items_ro              = (const u8*)(base + sizeof(stream_header_t) + (sizeof(u64) * ctx->m_header_ro->m_ids_capacity) + (sizeof(u32) * ctx->m_header_ro->m_item_capacity));

        u16* ids_idx_array_sorted = g_allocate_array<u16>(ctx->m_allocator, ctx->m_header_ro->m_ids_capacity);
        for (i32 i = 0; i < ctx->m_header_ro->m_ids_count; i++)
            ids_idx_array_sorted[i] = i;

        ctx->m_ids_idx_array_sorted = ids_idx_array_sorted;

        // Sort the ids array
        nsort::sort(ids_idx_array_sorted, ctx->m_header_ro->m_ids_count, s_cmp_ids, ctx->m_ids_array_unsorted_ro);
    }

    // Public API
    stream_context_t* stream_open_ro(alloc_t* allocator, const char* abs_filepath, char name[64])
    {
        stream_context_t* ctx = new_stream_context(allocator);
        if (nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (nmmio::open_ro(ctx->m_mmstream, abs_filepath))
            {
                const u8* base          = (const u8*)nmmio::address_ro(ctx->m_mmstream);
                ctx->m_mmstream_base_rw = nullptr;
                ctx->m_mmstream_base_ro = base;

                const stream_header_t* stream_header = (const stream_header_t*)base;
                if (strncmp(stream_header->m_name, name, 64) == 0)
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

    stream_context_t* stream_open_rw(alloc_t* allocator, const char* abs_filepath, char name[64])
    {
        stream_context_t* ctx = new_stream_context(allocator);
        if (nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (nmmio::open_rw(ctx->m_mmstream, abs_filepath))
            {
                u8*              base          = (u8*)nmmio::address_rw(ctx->m_mmstream);
                stream_header_t* stream_header = (stream_header_t*)base;
                if (strncmp(stream_header->m_name, name, 64) == 0)
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

    stream_context_t* stream_create(alloc_t* allocator, const char* abs_filepath, char name[64], i32 max_ids, i32 average_item_data_size)
    {
        const i32 max_items = DSTREAM_MAX_FILESIZE / (sizeof(u32) + (6 + 2 + average_item_data_size));

        stream_context_t* ctx = new_stream_context(allocator);
        if (!nmmio::exists(ctx->m_mmstream, abs_filepath))
        {
            if (!nmmio::create_rw(ctx->m_mmstream, abs_filepath, DSTREAM_MAX_FILESIZE))
            {
                destroy_stream_context(ctx);
                return nullptr;
            }
        }

        u8*              base          = (u8*)nmmio::address_rw(ctx->m_mmstream);
        stream_header_t* stream_header = (stream_header_t*)base;

        // Initialize the stream
        strncpy(stream_header->m_name, name, 64);
        stream_header->m_name[63]      = '\0';
        stream_header->m_time_begin    = 0;
        stream_header->m_time_end      = 0;
        stream_header->m_ids_count     = 0;
        stream_header->m_ids_capacity  = max_ids;
        stream_header->m_item_count    = 0;
        stream_header->m_item_capacity = max_items;

        stream_init_toc_rw(ctx, stream_header, base);

        // Initialize write offset
        stream_header->m_item_write_offset = ((u8*)ctx->m_item_offsets - base);

        nmmio::sync(ctx->m_mmstream);
        return ctx;
    }

    bool stream_close(stream_context_t* ctx)
    {
        if (ctx)
        {
            nmmio::sync(ctx->m_mmstream);
            nmmio::close(ctx->m_mmstream);
            return true;
        }
        return false;
    }

    void stream_sync(stream_context_t* ctx) { nmmio::sync(ctx->m_mmstream); }

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
        if (ctx->m_header == nullptr)
            return nullptr;

        stream_iterator_t* iterator = g_construct<stream_iterator_t>(ctx->m_allocator);
        iterator->m_cur_item_index  = 0;
        iterator->m_max_item_index  = ctx->m_header_ro->m_item_count;
        iterator->m_stream_context  = ctx;
        ctx->m_num_active_iterators++;
        return iterator;
    }

    void stream_destroy_iterator(stream_context_t* ctx, stream_iterator_t* iterator)
    {
        if (iterator)
        {
            ctx->m_num_active_iterators--;
            g_destruct(ctx->m_allocator, iterator);
        }
    }

    // Move iterator forward or backward by items_to_advance
    void stream_iterator_advance(stream_iterator_t* iterator, i64 items_to_advance)
    {
        i64 new_index = (i64)iterator->m_cur_item_index + items_to_advance;
        if (new_index < 0)
            new_index = 0;
        if (new_index > (i64)iterator->m_max_item_index)
            new_index = (i64)iterator->m_max_item_index;
        iterator->m_cur_item_index = (i32)new_index;
    }

    bool stream_is_full(stream_context_t* ctx)
    {
        const stream_header_t* hdr = ctx->m_header_ro;
        return (hdr->m_item_write_offset >= DSTREAM_MAX_FILESIZE);
    }

    // Write an item to the stream
    bool stream_write_item(stream_context_t* ctx, u64 time, u64 id, const void* data, i32 size)
    {
        if (ctx->m_header == nullptr)
            return false;

        u8*              base          = (u8*)nmmio::address_rw(ctx->m_mmstream);
        stream_header_t* hdr           = ctx->m_header;
        const u32        item_offset   = (u32)(hdr->m_item_write_offset);
        stream_item_t*   item          = (stream_item_t*)(base + item_offset);
        u8*              item_data_ptr = (u8*)(base + item_offset + sizeof(stream_item_t));

        if (hdr->m_item_count == 0)
        {
            // First item, set stream start time
            hdr->m_time_begin = time;
        }

        if (hdr->m_item_write_offset + sizeof(stream_item_t) + size > DSTREAM_MAX_FILESIZE)
        {
            // Stream is full, cannot write more data
            hdr->m_item_write_offset = DSTREAM_MAX_FILESIZE;
            return false;
        }

        // Find or add ID
        i16 id_index = ctx->find_or_add_id(id);
        ASSERT(id_index >= 0 && id_index < hdr->m_ids_count);

        // Set item header
        item->set_time(time - hdr->m_time_begin);
        item->set_id_index((i16)id_index);

        // Copy item data
        memcpy(item_data_ptr, data, size);

        // Update item offset array
        ctx->m_item_offsets[hdr->m_item_count] = item_offset;

        // Update header
        hdr->m_item_count += 1;
        hdr->m_item_write_offset += (sizeof(stream_item_t) + size);

        // Update stream end time
        if (time > hdr->m_time_end)
            hdr->m_time_end = time;

        return true;
    }

    // Get item, item index is relative to iterator, this is technically a read-only operation.
    bool stream_iterator_get_item(stream_iterator_t* iterator, u64 item_index, u64& out_item_time, u64& out_item_id, const u8*& out_item_data, i32& out_item_size)
    {
        if (item_index < (u64)iterator->m_max_item_index)
        {
            const stream_context_t* ctx = iterator->m_stream_context;
            if (ctx->m_header_ro != nullptr)
            {
                const stream_header_t* hdr = ctx->m_header_ro;

                const u32            item_offset = ctx->m_item_offsets_ro[item_index];
                const u8*            item_ptr    = ctx->m_mmstream_base_ro + item_offset;
                const u8*            item_data   = item_ptr + sizeof(stream_item_t);
                const stream_item_t* header      = (stream_item_t*)item_ptr;

                out_item_time      = header->get_time(hdr->m_time_begin);
                const u16 id_index = header->get_id_index();
                out_item_id        = ctx->m_ids_array_unsorted_ro[id_index];

                if ((item_index + 1) < (u64)hdr->m_item_count)
                {
                    const u32 next_item_offset = ctx->m_item_offsets_ro[item_index + 1];
                    out_item_size              = (next_item_offset - item_offset) - sizeof(stream_item_t);
                }
                else
                {
                    out_item_size = (hdr->m_item_write_offset - item_offset) - sizeof(stream_item_t);
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
