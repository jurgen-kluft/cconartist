#ifndef __CCONARTIST_NETWORK_SERVER_H__
#define __CCONARTIST_NETWORK_SERVER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    struct us_loop;
    struct us_server;

    typedef void* server_id_t;
    typedef void* client_id_t;

    // Message callback: data is ephemeral, not owned by the callback
    typedef void (*us_msg_cb)(client_id_t client_id, const u8* data, uint_t len, void* user);
    typedef void (*us_client_cb)(client_id_t client_id, void* user);

    // Loop creation, run & destroy
    us_loop* us_loop_create(alloc_t* allocator);
    i32      us_loop_run(us_loop* loop, i32 timeout_ms);
    void     us_loop_destroy(us_loop* loop);

    // Create & manage server (UDP or TCP)
    server_id_t us_server_create(us_loop* loop, u8 type, u32 ip, u16 port, uint_t recv_buf_size, us_msg_cb on_msg, us_client_cb on_connect, us_client_cb on_disconnect, void* user);
    i32         us_server_close(us_loop* loop, server_id_t server_id);

    // Create & manage clients
    int_t us_client_send(client_id_t client_id, const u8* data, uint_t len);
    i32   us_client_close(us_loop* loop, client_id_t client_id);

    // UDP broadcast, returns 0 on success, -1 on failure (if no clients or error)
    i32 us_broadcast(us_loop* loop, u32 ip, u16 port, const u8* data, uint_t len);

}  // namespace ncore

#endif  // __CCONARTIST_NETWORK_SERVER_H__
