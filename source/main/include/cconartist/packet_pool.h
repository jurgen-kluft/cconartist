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
    typedef struct
    {
        connection_info_t *m_conn;
        size_t             m_size;
        char               m_data[MAX_PACKET_SIZE];
    } packet_t;

    typedef struct
    {
        packet_t  *m_packets;
        int16_t   *m_free_list;
        int16_t    m_top;
        size_t     m_capacity;
        uv_mutex_t m_mutex;
    } packet_pool_t;

    packet_pool_t *packet_pool_create(size_t pool_size);
    void           packet_pool_destroy(packet_pool_t *pool);
    packet_t      *packet_acquire(packet_pool_t *pool);
    void           packet_release(packet_pool_t *pool, packet_t *pkt);
}  // namespace ncore

#endif
