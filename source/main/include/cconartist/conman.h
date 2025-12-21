#ifndef __CCONARTIST_TCP_CON_MGR_H__
#define __CCONARTIST_TCP_CON_MGR_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    namespace nconartist
    {
        struct server_tcp_t;  // Forward declaration
    }  // namespace nconartist

    struct tcp_con_t
    {
        nconartist::server_tcp_t *m_server;         // Owner, the TCP server
        uv_tcp_t                 *m_handle;         // For TCP
        uint64_t                  m_last_active;    // Timestamp of last activity
        uint8_t                   m_remote_ip[16];  // IPv4 or IPv6 octet representation
        u16                       m_remote_port;    // Remote port that the client is connecting from
        u16                       m_local_port;     // Local port that the client is connecting to
        u8                        m_flags[4];       //

        inline void set_connected() { m_flags[1] = 1; }
        inline bool is_connected() const { return m_flags[1] == 1; }
        inline void set_disconnected() { m_flags[1] = 2; }
        inline bool is_disconnected() const { return m_flags[1] == 2; }
    };

    struct tcp_con_mgr_t;

    tcp_con_mgr_t *tcp_con_mgr_create(alloc_t *allocator, u32 max_capacity);
    void           tcp_con_mgr_destroy(tcp_con_mgr_t *mgr);
    tcp_con_t     *tcp_con_alloc(tcp_con_mgr_t *mgr);
    void           tcp_con_free(tcp_con_mgr_t *mgr, tcp_con_t *info);
}  // namespace ncore

#endif
