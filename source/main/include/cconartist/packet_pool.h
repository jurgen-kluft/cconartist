#ifndef __CCONARTIST_PACKET_POOL_H__
#define __CCONARTIST_PACKET_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"
#include "cconartist/conman.h"

namespace ncore
{
    class alloc_t;

    struct packet_pool_t;

    struct packet_t
    {
        tcp_con_t *m_conn;
        u32        m_data_size;
        u32        m_data_capacity;
        u8        *m_data;
    };

    packet_pool_t *packet_pool_create(alloc_t *allocator, u32 initial_pool_size, u32 max_pool_size, u16 packet_data_size);
    void           packet_pool_destroy(packet_pool_t *&pool);
    packet_t      *packet_acquire(packet_pool_t *pool);
    void           packet_release(packet_pool_t *pool, packet_t *pkt);
}  // namespace ncore

#endif
