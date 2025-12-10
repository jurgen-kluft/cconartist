#ifndef __CCONARTIST_STREAM_MANAGER_H__
#define __CCONARTIST_STREAM_MANAGER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    typedef s8          stream_type_t;
    const stream_type_t c_stream_type_invalid       = -1;
    const stream_type_t c_stream_type_fixed_u8      = 1;
    const stream_type_t c_stream_type_fixed_u16     = 2;
    const stream_type_t c_stream_type_fixed_data    = 3;
    const stream_type_t c_stream_type_variable_data = 4;

    // Public API

    struct stream_manager_t;
    stream_manager_t* stream_manager_create(alloc_t* allocator, i32 max_streams, const char* base_path);
    void              stream_manager_destroy(alloc_t* allocator, stream_manager_t*& manager);
    void              stream_manager_flush(stream_manager_t* manager);
    void              stream_manager_update(stream_manager_t* manager);

    typedef i32 stream_id_t;
    stream_id_t stream_register(stream_manager_t* m, stream_type_t stream_type, const char* name, u64 user_id, u16 user_type, u64 file_size = 1 * cGB, u32 sizeof_item = 0);

    // Write to the stream, returns false if failed, check by calling stream_is_full() to see if stream is full
    bool stream_write_data(stream_manager_t* m, stream_id_t stream_id, u64 time, const u8* data, u32 size);
    bool stream_write_u8(stream_manager_t* m, stream_id_t stream_id, u64 time, u8 value);
    bool stream_write_u16(stream_manager_t* m, stream_id_t stream_id, u64 time, u16 value);
    bool stream_write_u32(stream_manager_t* m, stream_id_t stream_id, u64 time, u32 value);
    bool stream_write_f32(stream_manager_t* m, stream_id_t stream_id, u64 time, f32 value);

    // Functions to obtain data from the stream, returns number of items read, or -1 if error
    // Requests may be crossing stream file boundaries, so the function will return less items than requested if the end of the stream is reached.
    // User should call multiple times until all requested items are gotten.
    // Item time is relative to time_begin of the stream (5 bytes can cover up to 34 years of time range with millisecond precision)
    // Item layout depends on stream type:
    //    In a u8 data stream the item layout : [u8[5] time_offset, u8 value]
    //    In a u16 data stream the item layout : [u8[5] time_offset, u16 value]
    //    In a u32 data stream the item layout : [u8[5] time_offset, u32 value]
    //    In a f32 data stream the item layout : [u8[5] time_offset, f32 value]
    //    In a fixed data stream the item layout : [u8[5] time_offset, u8[data_size] data]
    //    In a variable data stream the item layout : [u8[5] time_offset, u8[4] data_size, u8[data_size] data] (max item count to read is 1)
    bool stream_time_range(stream_manager_t* m, stream_id_t stream_id, u64& out_time_begin, u64& out_time_end);
    i32  stream_read(stream_manager_t* m, stream_id_t stream_id, u64 item_index, u32 item_count, void const*& item_array, u32& item_size);

    // We also provide a general way to iterate over items in any stream, but this is the only way for variable size data streams.
    struct stream_iterator_t
    {
        stream_id_t m_stream_id;
        u64         m_current_offset;
        u64         m_current_item;
        u64         m_total_items;
    };

    //
    bool stream_iterator_begin(stream_manager_t* m, stream_id_t stream_id, stream_iterator_t& out_iterator);
    bool stream_iterator_next(stream_manager_t* m, stream_iterator_t& iterator, u64& time_begin, u8 const*& out_data_ptr, u32& out_data_size);
    void stream_iterator_end(stream_manager_t* m, stream_iterator_t& iterator);

}  // namespace ncore

#endif
