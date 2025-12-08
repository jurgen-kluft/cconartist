#include "ccore/c_memory.h"
#include "cconartist/streamman.h"

#include <string.h>

namespace ncore
{
#define STREAM_PAGE_MAX_ITEMS (8192 - 5)
    // Each stream page is exactly 64KB in size
    struct stream_page_t
    {
        u64 m_time_begin;                    // Timestamp begin of this page
        u64 m_time_end;                      // Timestamp end of this page
        i64 m_prev_page;                     // Base+Offset to previous page (0 = none)
        i64 m_next_page;                     // Base+Ofset to next page (0 = none)
        i32 m_item_count;                    // Number of items currently in this page
        i32 m_size;                          // Size of this page in bytes
        i64 m_items[STREAM_PAGE_MAX_ITEMS];  // Offsets to items in the mmapped file
    };

    // Stream Item Layout
    struct stream_item_header_t
    {
        u32 m_time_high;
        u32 m_time_low;
        u32 m_id_high;
        u32 m_id_low;
        u32 m_size;
        // followed by data...
    };

    struct stream_context_t;

    struct stream_iterator_t
    {
        stream_context_t* m_stream_ctx;
        stream_page_t*    m_prev_page;
        stream_page_t*    m_curr_page;
        stream_page_t*    m_next_page;
        i64               m_current_item_index;
    };

    struct stream_context_t
    {
        nmmio::mappedfile_t* m_mmstream;
        stream_page_t*       m_current_page;
        int                  m_num_active_iterators;
    };

    void stream_page_initialize(u8 const* base, stream_page_t* page, stream_page_t* previous_page)
    {
        page->m_time_begin = 0;
        page->m_time_end   = 0;
        page->m_prev_page  = 0;
        page->m_next_page  = 0;
        page->m_item_count = 0;
        page->m_size       = 0;
        nmem::memset(page->m_items, 0, sizeof(page->m_items));
        if (previous_page)
        {
            page->m_prev_page          = (u8*)previous_page - base;
            previous_page->m_next_page = (u8*)page - base;
        }
    }

    stream_page_t* stream_get_current_page(stream_context_t* ctx) { return ctx->m_current_page; }

    stream_page_t* stream_get_previous_page(stream_context_t* ctx, stream_page_t* current_page)
    {
        if (current_page->m_prev_page == 0)
            return nullptr;
        u8* base_addr = (u8*)nmmio::address_rw(ctx->m_mmstream);
        return (stream_page_t*)(base_addr + current_page->m_prev_page);
    }

    stream_page_t* stream_get_next_page(stream_context_t* ctx, stream_page_t* current_page)
    {
        if (current_page->m_next_page == 0)
            return nullptr;
        u8* base_addr = (u8*)nmmio::address_rw(ctx->m_mmstream);
        return (stream_page_t*)(base_addr + current_page->m_next_page);
    }

    // Public API
    stream_context_t* stream_create(const char* path)
    {
        stream_context_t* ctx = new stream_context_t();
        ctx->m_mmstream       = nullptr;
        nmmio::allocate(nullptr, ctx->m_mmstream);
        if (!nmmio::open_rw(ctx->m_mmstream, path))
        {
            nmmio::deallocate(nullptr, ctx->m_mmstream);
            delete ctx;
            return nullptr;
        }
        ctx->m_current_page         = (stream_page_t*)nmmio::address_rw(ctx->m_mmstream);
        ctx->m_num_active_iterators = 0;

        // TODO: figure out last page in the stream for appending

        return ctx;
    }

    void stream_write_item(stream_context_t* ctx, u64 time, u64 id, const void* data, i32 size)
    {
        stream_page_t* current_page = stream_get_current_page(ctx);
        u8*            base         = (u8*)nmmio::address_rw(ctx->m_mmstream);

        if (current_page == nullptr)
        {
            stream_page_t* first_page = (stream_page_t*)base;
            stream_page_initialize(base, first_page, nullptr);
            ctx->m_current_page = first_page;
        }

        // Write item data at the end of the page
        i64 item_offset = current_page->m_size;
        u8* item_addr   = base + item_offset;

        if (time > current_page->m_time_end)
            current_page->m_time_end = time;

        // Write item header
        stream_item_header_t* item_header = (stream_item_header_t*)item_addr;
        item_header->m_time_high          = (u32)(time >> 32);
        item_header->m_time_low           = (u32)(time & 0xFFFFFFFF);
        item_header->m_id_high            = (u32)(id >> 32);
        item_header->m_id_low             = (u32)(id & 0xFFFFFFFF);
        item_header->m_size               = (u32)size;

        // Write item data
        u8* item_data = (u8*)item_header + sizeof(stream_item_header_t);
        nmem::memcpy(item_data, data, size);

        // Update page metadata
        current_page->m_items[current_page->m_item_count++] = item_offset;

        // Check if we need to allocate a new page
        if (current_page->m_item_count >= STREAM_PAGE_MAX_ITEMS)
        {
            // This page is full, allocate a new page, and a page needs to be aligned to 8 bytes
            const u64 next_item_offset = ((u64)(item_data + size) + (8 - 1)) & ~(8 - 1);  // Align to 8 bytes
            current_page->m_size       = next_item_offset;
            stream_page_t* new_page    = (stream_page_t*)(base + next_item_offset);
            stream_page_initialize(base, new_page, current_page);
            ctx->m_current_page = new_page;
        }
        else
        {
            // Update current page size for the next item
            current_page->m_size = ((u64)(item_data + size) + (4 - 1)) & ~(4 - 1);  // Align to 4 bytes
        }
    }

    void stream_sync(stream_context_t* ctx) { nmmio::sync(ctx->m_mmstream); }

    void stream_destroy(stream_context_t* ctx)
    {
        if (ctx)
        {
            ASSERT(ctx->m_num_active_iterators == 0);
            if (ctx->m_mmstream)
            {
                nmmio::sync(ctx->m_mmstream);
                nmmio::close(ctx->m_mmstream);
                nmmio::deallocate(nullptr, ctx->m_mmstream);
            }
            delete ctx;
        }
    }

    // Create a stream iterator
    stream_iterator_t* stream_create_iterator(stream_context_t* ctx)
    {
        stream_iterator_t* iterator    = new stream_iterator_t();
        iterator->m_stream_ctx         = ctx;
        iterator->m_curr_page          = stream_get_current_page(ctx);
        iterator->m_prev_page          = stream_get_previous_page(ctx, iterator->m_curr_page);
        iterator->m_next_page          = stream_get_next_page(ctx, iterator->m_curr_page);
        iterator->m_current_item_index = -1;
        ctx->m_num_active_iterators++;
        return iterator;
    }

    void stream_destroy_iterator(stream_context_t* ctx, stream_iterator_t* iterator)
    {
        if (iterator)
        {
            ctx->m_num_active_iterators--;
            delete iterator;
        }
    }

    bool stream_iterator_next_page(stream_iterator_t* iterator)
    {
        // Make sure that we always have a current page
        if (iterator->m_next_page)
        {
            iterator->m_prev_page = iterator->m_curr_page;
            iterator->m_curr_page = iterator->m_next_page;
            iterator->m_next_page = stream_get_next_page(iterator->m_stream_ctx, iterator->m_curr_page);
            return true;
        }
        return false;
    }

    bool stream_iterator_previous_page(stream_iterator_t* iterator)
    {
        // Make sure that we always have a current page
        if (iterator->m_prev_page)
        {
            iterator->m_next_page = iterator->m_curr_page;
            iterator->m_curr_page = iterator->m_prev_page;
            iterator->m_prev_page = stream_get_previous_page(iterator->m_stream_ctx, iterator->m_prev_page);
            return true;
        }
        return false;
    }

    // Move iterator forward or backward by items_to_advance
    void stream_iterator_advance(stream_iterator_t* iterator, i64 items_to_advance)
    {
        if (iterator->m_curr_page == nullptr)
            return;

        if (items_to_advance > 0)
        {
            items_to_advance += iterator->m_current_item_index;
            iterator->m_current_item_index = 0;

            while (items_to_advance >= iterator->m_curr_page->m_item_count)
            {
                if (!stream_iterator_next_page(iterator))
                {
                    // Reached end of stream
                    items_to_advance = items_to_advance % iterator->m_curr_page->m_item_count;
                    break;
                }
                items_to_advance -= iterator->m_curr_page->m_item_count;
            }
            iterator->m_current_item_index += items_to_advance;
        }
        else if (items_to_advance < 0)
        {
            items_to_advance += iterator->m_current_item_index;
            iterator->m_current_item_index = 0;

            while (items_to_advance < 0)
            {
                if (!stream_iterator_previous_page(iterator))
                {
                    // Reached beginning of stream
                    items_to_advance = items_to_advance % iterator->m_curr_page->m_item_count;
                    break;
                }
                items_to_advance += iterator->m_curr_page->m_item_count;
            }
            iterator->m_current_item_index += items_to_advance;
        }
    }

    // Get item, item index is relative to iterator
    bool stream_iterator_get_item(stream_iterator_t* iterator, u64 item_index, u64& out_item_time, u64& out_item_id, void*& out_item_data, i32& out_item_size)
    {
        if (iterator->m_curr_page == nullptr)
            return false;

        i64 target_index = iterator->m_current_item_index + item_index;

        // Navigate to the correct page
        stream_page_t* page = iterator->m_curr_page;
        while (target_index >= page->m_item_count)
        {
            if (!stream_iterator_next_page(iterator))
                return false;  // Reached end of stream
            target_index -= page->m_item_count;
            page = iterator->m_curr_page;
        }

        // Retrieve the item
        u8*  base        = (u8*)nmmio::address_rw(iterator->m_stream_ctx->m_mmstream);
        i64  item_offset = page->m_items[target_index];
        u8*  item_addr   = base + item_offset;
        u32* item_header = (u32*)item_addr;
        out_item_time    = ((u64)item_header[0] << 32) | (u64)item_header[1];
        out_item_id      = ((u64)item_header[2] << 32) | (u64)item_header[3];
        out_item_size    = (i32)item_header[4];

        u8* item_data = item_addr + sizeof(u32) * 5;
        out_item_data = item_data;

        return true;
    }

}  // namespace ncore
