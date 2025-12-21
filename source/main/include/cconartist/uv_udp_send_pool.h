#ifndef __CCONARTIST_UV_UDP_SEND_POOL_H__
#define __CCONARTIST_UV_UDP_SEND_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct uv_udp_send_pool_t;

    // Initialize pool
    uv_udp_send_pool_t *uv_udp_send_pool_create(alloc_t *allocator, u16 size);
    void                uv_udp_send_pool_destroy(uv_udp_send_pool_t *&send_pool);

    // Acquire a send request from the pool
    uv_udp_send_t *uv_udp_send_acquire(uv_udp_send_pool_t *send_pool);

    // Release a send request back to the pool
    void uv_udp_send_release(uv_udp_send_pool_t *send_pool, uv_udp_send_t *req);

}  // namespace ncore

#endif
