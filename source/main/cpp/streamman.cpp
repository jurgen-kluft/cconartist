#include "ccore/c_memory.h"
#include "cconartist/streamman.h"

#include <string.h>

namespace ncore
{
    // Each stream page is exactly 64KB in size
    struct stream_page_t
    {
        u64 m_time_begin;       // Timestamp begin of this page
        u64 m_time_end;         // Timestamp end of this page
        i64 m_previous;         // Offset to previous page (0 = none)
        i64 m_next;             // Ofset to next page (0 = none)
        i64 m_item_count;       // Number of items currently in this page
        i64 m_items[8192 - 5];  // Offsets to items in the mmapped file
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

    void stream_page_initialize(stream_page_t* page, stream_page_t* previous_page)
    {
        page->m_time_begin = 0;
        page->m_time_end   = 0;
        page->m_previous   = 0;
        page->m_next       = 0;
        page->m_item_count = 0;
        nmem::memset(page->m_items, 0, sizeof(page->m_items));
        if (previous_page)
        {
            page->m_previous      = ((u8*)previous_page - (u8*)page);
            previous_page->m_next = (u8*)page - (u8*)previous_page;
        }
    }

    stream_page_t* stream_get_current_page(stream_context_t* ctx) { return ctx->m_current_page; }

    stream_page_t* stream_get_previous_page(stream_context_t* ctx, stream_page_t* current_page)
    {
        if (current_page->m_previous == 0)
            return nullptr;
        u8* base_addr = (u8*)nmmio::address_rw(ctx->m_mmstream);
        return (stream_page_t*)(base_addr + current_page->m_previous);
    }

    stream_page_t* stream_get_next_page(stream_context_t* ctx, stream_page_t* current_page)
    {
        if (current_page->m_next == 0)
            return nullptr;
        u8* base_addr = (u8*)nmmio::address_rw(ctx->m_mmstream);
        return (stream_page_t*)(base_addr + current_page->m_next);
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
        if (current_page == nullptr)
        {
            current_page = (stream_page_t*)nmmio::address_rw(ctx->m_mmstream);
            stream_page_initialize(current_page, nullptr);
            ctx->m_current_page = current_page;
        }
        else if (current_page->m_item_count >= (i64)DARRAYSIZE(current_page->m_items))
        {
            // Allocate a new page
            u8*            base_addr = (u8*)nmmio::address_rw(ctx->m_mmstream);
            stream_page_t* new_page  = (stream_page_t*)(base_addr + ((u8*)current_page - base_addr) + sizeof(stream_page_t));
            stream_page_initialize(new_page, current_page);
            ctx->m_current_page = new_page;
            current_page        = new_page;
        }

        // Write item data at the end of the page
        u8* item_addr = nullptr;
        if (current_page->m_item_count == 0)
        {
            item_addr = (u8*)current_page + sizeof(stream_page_t);

            current_page->m_time_begin = time;
            current_page->m_time_end   = time;
        }
        else
        {
            i64 last_item_offset = current_page->m_items[current_page->m_item_count - 1];
            u8* last_item_addr   = (u8*)current_page + last_item_offset;
            i32 last_item_size   = *((i32*)(last_item_addr));
            item_addr            = last_item_addr + sizeof(i32) + last_item_size;

            if (time > current_page->m_time_end)
                current_page->m_time_end = time;
        }

        // Write item header
        u64* item_header = (u64*)item_addr;
        item_header[0]   = time;
        item_header[1]   = id;

        // Write item data
        u8* item_data = item_addr + sizeof(u64) * 2;
        nmem::memcpy(item_data, data, size);

        // Update page metadata
        current_page->m_items[current_page->m_item_count++] = (i8*)item_addr - (i8*)current_page;
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
        i64 item_offset = page->m_items[target_index];
        u8* item_addr   = (u8*)page + item_offset;
        u64* item_header = (u64*)item_addr;
        out_item_time   = item_header[0];
        out_item_id     = item_header[1];
        u8* item_data   = item_addr + sizeof(u64) * 2;

        // The size can be derived from the next item's offset, we always fill in the next item's offset since we
        // need to know where the next item starts. If this is the last item, we can calculate the size from using
        //
        out_item_size = *((i32*)(item_data - sizeof(i32)));
        out_item_data = (void*)item_data;

        return true;
    }

}  // namespace ncore
