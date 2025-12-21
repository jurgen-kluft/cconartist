#ifndef __CCONARTIST_TCP_WRITE_POOL_H__
#define __CCONARTIST_TCP_WRITE_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct uv_tcp_write_pool_t;

    // Initialize pool
    uv_tcp_write_pool_t* uv_tcp_write_pool_create(alloc_t* allocator, u16 capacity);
    void                 uv_tcp_write_pool_destroy(uv_tcp_write_pool_t*& write_pool);
    uv_write_t*          uv_tcp_write_acquire(uv_tcp_write_pool_t* write_pool);
    void                 uv_tcp_write_release(uv_tcp_write_pool_t* write_pool, uv_write_t* req);

}  // namespace ncore
#endif
