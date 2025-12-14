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

    struct stream_manager_t;

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
        stream_manager_t       *m_stream_man;     // For streaming packets to disk
        uint64_t                m_last_active;    // Timestamp of last activity
        uint8_t                 m_remote_ip[16];  // IPv4 or IPv6 octet representation
        int                     m_remote_port;    // Remote port that the client is connecting from
        int                     m_local_port;     // Local port that the server is listening on
        connection_state_t      m_state;          // Connected or Disconnected
        connection_type_t       m_type;           // TCP or UDP
        uv_tcp_t               *m_handle;         // For TCP
        struct sockaddr_storage m_remote_addr;    // For UDP
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
