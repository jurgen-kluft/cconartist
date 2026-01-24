#ifndef __CCONARTIST_UNIX_SOCKET_SERVER_H__
#define __CCONARTIST_UNIX_SOCKET_SERVER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    struct us_loop;
    struct us_server;

    // Message callback: data is ephemeral, not owned by the callback
    typedef void (*us_msg_cb)(i32 server_id, i32 client_fd, const void* data, uint_t len, void* user);
    typedef void (*us_client_cb)(i32 server_id, i32 client_fd, void* user);

    // Loop creation, run & destroy
    us_loop* us_loop_create(alloc_t* allocator);
    i32      us_loop_run(us_loop* loop, i32 timeout_ms);
    void     us_loop_destroy(us_loop* loop);

    // Create & manage server
    i32 us_server_create(us_loop* loop, const char* socket_path, uint_t path_len, uint_t recv_buf_size, us_msg_cb on_msg, us_client_cb on_connect, us_client_cb on_disconnect, void* user);
    i32 us_server_close(us_loop* loop, i32 server_id);

    // Utilities
    i32   us_broadcast(us_loop* loop, i32 server_id, const void* data, uint_t len);
    int_t us_client_send(i32 client_fd, const void* data, uint_t len);
    i32   us_client_close(us_loop* loop, i32 server_id, i32 client_fd);

}  // namespace ncore

#endif  // __CCONARTIST_UNIX_SOCKET_SERVER_H__
