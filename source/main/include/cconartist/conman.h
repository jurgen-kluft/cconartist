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
        struct server_t;   // Forward declaration
        struct servers_t;  // Forward declaration
    }  // namespace nconartist

    struct stream_context_t;

    enum connection_type_t
    {
        CONN_TCP,
        CONN_UDP
    };

    enum connection_state_t
    {
        STATE_CONNECTED,
        STATE_DISCONNECTED
    };

    struct connection_info_t
    {
        nconartist::server_t   *m_server;         // Back reference to server
        uint8_t                 m_remote_ip[16];  // IPv4 or IPv6 octet representation
        int                     m_remote_port;
        int                     m_local_port;
        connection_type_t       m_type;
        connection_state_t      m_state;
        uint64_t                m_last_active;
        uv_tcp_t               *m_handle;       // For TCP
        struct sockaddr_storage m_remote_addr;  // For UDP
        stream_context_t       *m_stream_ctx;   // For streaming packets to disk
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
