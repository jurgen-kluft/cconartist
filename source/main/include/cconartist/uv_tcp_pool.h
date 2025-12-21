#ifndef __CCONARTIST_UV_TCP_POOL_H__
#define __CCONARTIST_UV_TCP_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct uv_tcp_pool_t;

    // Initialize pool
    uv_tcp_pool_t* uv_tcp_pool_create(alloc_t* allocator, u32 capacity);
    void           uv_tcp_pool_destroy(uv_tcp_pool_t*& write_pool);
    uv_tcp_t*      uv_tcp_acquire(uv_tcp_pool_t* write_pool);
    void           uv_tcp_release(uv_tcp_pool_t* write_pool, uv_tcp_t* uv);

}  // namespace ncore

#endif
