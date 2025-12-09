#ifndef __CCONARTIST_PACKET_POOL_H__
#define __CCONARTIST_PACKET_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"
#include "cconartist/conman.h"

#define MAX_PACKET_SIZE 500

namespace ncore
{
    class alloc_t;

    struct packet_t
    {
        connection_info_t *m_conn;
        u32                m_size;
        char               m_data[MAX_PACKET_SIZE];
    };

    struct packet_pool_t
    {
        packet_pool_t()
            : m_packets(nullptr)
            , m_free_list(nullptr)
            , m_top(-1)
            , m_capacity(0)
            , m_allocator(nullptr)
        {
            m_mutex = {};
        }

        packet_t  *m_packets;
        u16       *m_free_list;
        i32        m_top;
        i32        m_capacity;
        uv_mutex_t m_mutex;
        alloc_t   *m_allocator;
    };

    void      packet_pool_init(alloc_t *allocator, u16 pool_size, packet_pool_t &pool);
    void      packet_pool_release(packet_pool_t &pool);
    packet_t *packet_acquire(packet_pool_t &pool);
    void      packet_release(packet_pool_t &pool, packet_t *pkt);
}  // namespace ncore

#endif
