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

    struct tcp_write_pool_t;

    // Initialize pool
    tcp_write_pool_t* write_pool_create(alloc_t* allocator, u16 capacity);
    void              write_pool_destroy(tcp_write_pool_t*& write_pool);
    uv_write_t*       write_pool_acquire(tcp_write_pool_t* write_pool);
    void              write_pool_release(tcp_write_pool_t* write_pool, uv_write_t* req);

}  // namespace ncore
#endif
