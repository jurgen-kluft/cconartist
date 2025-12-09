#ifndef __CCONARTIST_STREAM_MANAGER_H__
#define __CCONARTIST_STREAM_MANAGER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    struct stream_context_t;
    struct stream_iterator_t;
    struct stream_const_iterator_t;

    // Public API
    stream_context_t* stream_open_ro(alloc_t* allocator, const char* abs_filepath, const char* name);
    stream_context_t* stream_open_rw(alloc_t* allocator, const char* abs_filepath, const char* name);
    stream_context_t* stream_create(alloc_t* allocator, const char* abs_filepath, const char* name, u16 max_ids = 8192, u32 average_item_data_size = 4);
    bool              stream_is_full(stream_context_t* ctx);
    void              stream_sync(stream_context_t* ctx);
    bool              stream_close(stream_context_t*& ctx);

    // Write an item to the stream, returns false if failed, check by calling stream_is_full() to see if stream is full
    bool stream_write_item(stream_context_t* ctx, u64 time, u64 id, const void* data, u32 size);

    // Create a stream iterator (non-thread safe)
    stream_iterator_t* stream_create_iterator(stream_context_t* ctx);
    void               stream_destroy_iterator(stream_context_t* ctx, stream_iterator_t* iterator);

    // Move iterator forward or backward by items_to_advance
    void stream_iterator_advance(stream_iterator_t* iterator, i64 items_to_advance);

    // Get item, item index is relative to iterator
    bool stream_iterator_get_item(stream_iterator_t* iterator, u64 relative_item_index, u64& out_item_time, u64& out_item_id, const u8*& out_item_data, u32& out_item_size);

}  // namespace ncore

#endif
