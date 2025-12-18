#ifndef __CCONARTIST_CONMANAGER_H__
#define __CCONARTIST_CONMANAGER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    namespace nconartist
    {
        struct server_tcp_t;  // Forward declaration
        struct server_udp_t;  // Forward declaration
        struct servers_t;     // Forward declaration
    }  // namespace nconartist

    struct stream_manager_t;

    struct connection_info_t
    {
        nconartist::server_tcp_t *m_server;         // Back reference to server
        uint64_t                  m_last_active;    // Timestamp of last activity
        uint8_t                   m_remote_ip[16];  // IPv4 or IPv6 octet representation
        u16                       m_remote_port;    // Remote port that the client is connecting from
        u16                       m_local_port;     // Local port that the client is connecting to
        uv_tcp_t                 *m_handle;         // For TCP
        struct sockaddr_storage   m_remote_addr;    // For UDP
        u8                        m_flags[4];
        inline void               set_tcp() { m_flags[0] = 1; }
        inline bool               is_tcp() const { return m_flags[0] == 1; }
        inline void               set_udp() { m_flags[0] = 2; }
        inline bool               is_udp() const { return m_flags[0] == 2; }
        inline void               set_connected() { m_flags[1] = 1; }
        inline bool               is_connected() const { return m_flags[1] == 1; }
        inline void               set_disconnected() { m_flags[1] = 2; }
        inline bool               is_disconnected() const { return m_flags[1] == 2; }
    };

    struct connection_manager_t
    {
        connection_info_t **m_connections;
        connection_info_t  *m_free_connection;
        int                 m_count;
        int                 m_capacity;
        uv_mutex_t          m_mutex;
    };

    int                connection_manager_init(connection_manager_t *mgr, size_t initial_capacity);
    void               connection_manager_destroy(connection_manager_t *mgr);
    connection_info_t *connection_manager_alloc(connection_manager_t *mgr);
    connection_info_t *connection_manager_commit(connection_manager_t *mgr, connection_info_t *info);
    void               connection_manager_mark_disconnected(connection_manager_t *mgr, connection_info_t *info);

}  // namespace ncore

#endif
