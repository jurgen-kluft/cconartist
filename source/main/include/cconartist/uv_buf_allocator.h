#ifndef __CCONARTIST_UV_BUF_POOL_H__
#define __CCONARTIST_UV_BUF_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct uv_buf_alloc_t;

    uv_buf_alloc_t* uv_buf_alloc_create(alloc_t* allocator, u32 initial_capacity, u32 max_capacity);
    void            uv_buf_alloc_destroy(uv_buf_alloc_t*& alloc);
    byte*           uv_buf_alloc(uv_buf_alloc_t* alloc, u32 buf_size);
    void            uv_buf_release(uv_buf_alloc_t* alloc, byte* buf_data);

}  // namespace ncore

#endif
