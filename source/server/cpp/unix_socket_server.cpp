#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>

#include "ccore/c_allocator.h"
#include "cconartist/unix_socket_server.h"

namespace ncore
{
    struct us_loop;
    struct us_server;

#define US_KIND_SERVER   0x53525652  // "SRVR"
#define US_KIND_CONN     0x434F4E4E  // "CONN"
#define US_MESSAGE_MAGIC 0x55434F4E  // "UCON"

#define US_MAX_KEVENT_BATCH 128
#define US_SERVER_CONN_CAP  64
#define US_LOOP_SERVER_CAP  32

// Fixed 32 byte message header
#define US_MESSAGE_HEADER_SIZE  32
#define US_MESSAGE_OFFSET_MAGIC 0
#define US_MESSAGE_OFFSET_LEN   4
#define US_MESSAGE_OFFSET_TYPE  8
#define US_MESSAGE_OFFSET_FLAGS 12

    struct us_conn
    {
        i32        m_kind;
        i32        m_fd;
        us_server* m_server;
        byte*      m_buf;
        u32        m_cap;
        u32        m_used;
    };

    struct us_server
    {
        i32          m_kind;
        i32          m_server_id;
        i32          m_listen_fd;
        char*        m_path;
        u32          m_path_len;
        i32          m_active;
        u32          m_recv_buf_size;
        us_msg_cb    m_on_msg;
        us_client_cb m_on_connect;
        us_client_cb m_on_disconnect;
        void*        m_user;
        us_conn*     m_conns;
        u32          m_conns_count;
        u32          m_conns_cap;
    };

    struct us_loop
    {
        i32        m_kq;
        i32        m_stop;
        i32        m_next_server_id;
        us_server* m_servers;
        u32        m_servers_count;
        u32        m_servers_cap;
        alloc_t*   m_allocator;
    };

    static i32 set_fd_nonblock(i32 fd)
    {
        i32 flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0)
            return -1;
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
            return -1;
        return 0;
    }

    static i32 set_fd_cloexec(i32 fd)
    {
        i32 flags = fcntl(fd, F_GETFD, 0);
        if (flags < 0)
            return -1;
        if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
            return -1;
        return 0;
    }

    static i32 set_fd_nosigpipe(i32 fd)
    {
        i32 one = 1;
        return setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
    }

    static i32 kq_add_read(i32 kq, i32 fd, void* udata)
    {
        struct kevent kev;
        EV_SET(&kev, (uintptr_t)fd, EVFILT_READ, EV_ADD, 0, 0, udata);
        return kevent(kq, &kev, 1, NULL, 0, NULL);
    }

    static i32 kq_del_read(i32 kq, i32 fd)
    {
        struct kevent kev;
        EV_SET(&kev, (uintptr_t)fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        return kevent(kq, &kev, 1, NULL, 0, NULL);
    }

    static void close_conn(us_loop* L, us_server* server, uint_t idx)
    {
        us_conn* connection = &server->m_conns[idx];
        i32      cfd        = connection->m_fd;
        if (connection->m_fd >= 0)
        {
            kq_del_read(L->m_kq, connection->m_fd);
            close(connection->m_fd);
        }
        if (server->m_on_disconnect)
        {
            server->m_on_disconnect(server->m_server_id, cfd, server->m_user);
        }
        if (connection->m_buf)
        {
            g_deallocate_array(L->m_allocator, connection->m_buf);
        }

        uint_t last = server->m_conns_count - 1;
        if (idx != last)
        {
            server->m_conns[idx] = server->m_conns[last];
            us_conn* moved       = &server->m_conns[idx];
            kq_del_read(L->m_kq, moved->m_fd);
            kq_add_read(L->m_kq, moved->m_fd, (void*)moved);
        }
        memset(&server->m_conns[last], 0, sizeof(us_conn));
        server->m_conns_count--;
    }

    static i32 accept_new_clients(us_loop* L, us_server* server)
    {
        for (;;)
        {
            i32 cfd = accept(server->m_listen_fd, NULL, NULL);
            if (cfd < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return 0;
                if (errno == EINTR)
                    continue;
                return -1;
            }
            if (set_fd_nonblock(cfd) < 0 || set_fd_cloexec(cfd) < 0 || set_fd_nosigpipe(cfd) < 0)
            {
                close(cfd);
                continue;
            }
            if (server->m_conns_count >= server->m_conns_cap)
            {
                errno = 12;  // ENOMEM
                close(cfd);
                continue;
            }
            us_conn* connection = &server->m_conns[server->m_conns_count++];
            memset(connection, 0, sizeof(*connection));
            connection->m_kind   = US_KIND_CONN;
            connection->m_server = server;
            connection->m_fd     = cfd;
            connection->m_cap    = server->m_recv_buf_size;
            connection->m_buf    = g_allocate_array<byte>(L->m_allocator, connection->m_cap);
            connection->m_used   = 0;
            if (!connection->m_buf)
            {
                close_conn(L, server, server->m_conns_count - 1);
                continue;
            }
            if (kq_add_read(L->m_kq, cfd, (void*)connection) < 0)
            {
                close_conn(L, server, server->m_conns_count - 1);
                continue;
            }
            if (server->m_on_connect)
                server->m_on_connect(server->m_server_id, cfd, server->m_user);
        }
    }

    static u32 read_u32_be(const byte* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | ((u32)p[3]); }

    static i32 read_from_client(us_loop* L, us_conn* connection)
    {
        CC_UNUSED(L);
        for (;;)
        {
            // Header
            u32 hn = 0;
            while (hn < US_MESSAGE_HEADER_SIZE)
            {
                ssize_t n = read(connection->m_fd, connection->m_buf + hn, US_MESSAGE_HEADER_SIZE - hn);
                if (n > 0)
                {
                    hn += (u32)n;
                    continue;
                }
                if (n == 0)
                    return -1;  // EOF
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return 0;  // No more data for now
                if (errno == EINTR)
                    continue;
                return -1;
            }

            const u32 magic = read_u32_be(connection->m_buf + US_MESSAGE_OFFSET_MAGIC);
            if (magic != US_MESSAGE_MAGIC)
                return -1;

            const u32 payload_len = read_u32_be(connection->m_buf + US_MESSAGE_OFFSET_LEN);
            if (payload_len > connection->m_cap - US_MESSAGE_HEADER_SIZE)
                return -1;

            // Now read the payload of the message
            u32 bn = 0;
            while (bn < payload_len)
            {
                ssize_t n = read(connection->m_fd, connection->m_buf + US_MESSAGE_HEADER_SIZE + bn, payload_len - bn);
                if (n > 0)
                {
                    bn += (u32)n;
                    continue;
                }
                if (n == 0)
                    return -1;  // EOF
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return 0;  // No more data for now
                if (errno == EINTR)
                    continue;
                return -1;
            }

            // Full message read, now dispatch
            connection->m_used = US_MESSAGE_HEADER_SIZE + payload_len;
            connection->m_server->m_on_msg(connection->m_server->m_server_id, connection->m_fd, (const void*)connection->m_buf, connection->m_used, connection->m_server->m_user);
            connection->m_used = 0;
        }
    }

    us_loop* us_loop_create(alloc_t* mi)
    {
        if (!mi)
        {
            errno = 22;
            return NULL;
        }
        i32 kq = kqueue();
        if (kq < 0)
            return NULL;
        us_loop* loop = g_allocate<us_loop>(mi);
        if (!loop)
        {
            close(kq);
            return NULL;
        }
        memset(loop, 0, sizeof(*loop));
        loop->m_kq             = kq;
        loop->m_stop           = 0;
        loop->m_next_server_id = 1;
        loop->m_servers        = g_allocate_array_and_clear<us_server>(mi, US_LOOP_SERVER_CAP);
        loop->m_servers_count  = 0;
        loop->m_servers_cap    = US_LOOP_SERVER_CAP;
        loop->m_allocator      = mi;
        return loop;
    }

    static us_server* find_server_by_id(us_loop* L, i32 server_id, uint_t* out_idx)
    {
        for (uint_t i = 0; i < L->m_servers_count; ++i)
        {
            if (L->m_servers[i].m_active && L->m_servers[i].m_server_id == server_id)
            {
                if (out_idx)
                    *out_idx = i;
                return &L->m_servers[i];
            }
        }
        return NULL;
    }

    i32 us_server_create(us_loop* loop, const char* socket_path, uint_t path_len, uint_t recv_buf_size, us_msg_cb on_msg, us_client_cb on_connect, us_client_cb on_disconnect, void* user)
    {
        if (!loop || !socket_path || path_len == 0 || !on_msg || recv_buf_size == 0)
        {
            errno = 22;  // EINVAL
            return -1;
        }

        if (path_len >= sizeof(((sockaddr_un*)0)->sun_path))
        {
            errno = 63;  // ENAMETOOLONG
            return -1;
        }

        if (loop->m_servers_count >= loop->m_servers_cap)
        {
            errno = 12;  // ENOMEM
            return -1;
        }

        i32 fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0)
            return -1;
        if (set_fd_nonblock(fd) < 0 || set_fd_cloexec(fd) < 0)
        {
            i32 e = errno;
            close(fd);
            errno = e;
            return -1;
        }

        sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        memcpy(addr.sun_path, socket_path, path_len);
        addr.sun_path[path_len] = '\0';

        unlink(addr.sun_path);

        if (bind(fd, (sockaddr*)&addr, (socklen_t)sizeof(addr)) < 0)
        {
            i32 e = errno;
            close(fd);
            errno = e;
            return -1;
        }

        if (listen(fd, backlog > 0 ? backlog : 64) < 0)
        {
            i32 e = errno;
            close(fd);
            errno = e;
            return -1;
        }

        us_server* server = &loop->m_servers[loop->m_servers_count++];
        memset(server, 0, sizeof(*server));
        server->m_kind          = US_KIND_SERVER;
        server->m_server_id     = loop->m_next_server_id++;
        server->m_listen_fd     = fd;
        server->m_recv_buf_size = recv_buf_size;
        server->m_on_msg        = on_msg;
        server->m_on_connect    = on_connect;
        server->m_on_disconnect = on_disconnect;
        server->m_user          = user;
        server->m_conns         = g_allocate_array_and_clear<us_conn>(loop->m_allocator, US_SERVER_CONN_CAP);
        server->m_conns_count   = 0;
        server->m_conns_cap     = US_SERVER_CONN_CAP;
        server->m_active        = 1;

        server->m_path = g_allocate_array<char>(loop->m_allocator, path_len + 1);
        if (!server->m_path)
        {
            i32 e = errno;
            close(fd);
            errno = e;
            return -1;
        }
        memcpy(server->m_path, socket_path, path_len);
        server->m_path[path_len] = '\0';
        server->m_path_len       = path_len;

        if (kq_add_read(loop->m_kq, fd, (void*)server) < 0)
        {
            i32 e = errno;
            close(fd);
            g_deallocate_array(loop->m_allocator, server->m_path);
            memset(server, 0, sizeof(*server));
            loop->m_servers_count--;
            errno = e;
            return -1;
        }

        return server->m_server_id;
    }

    i32 us_server_close(us_loop* loop, i32 server_id)
    {
        if (!loop)
        {
            errno = 22;
            return -1;
        }
        uint_t     idx    = 0;
        us_server* server = find_server_by_id(loop, server_id, &idx);
        if (!server)
        {
            errno = 2;
            return -1;
        }

        while (server->m_conns_count > 0)
        {
            close_conn(loop, server, server->m_conns_count - 1);
        }
        if (server->m_conns)
        {
            // us_dealloc(loop, server->m_conns);
            g_deallocate_array(loop->m_allocator, server->m_conns);
            server->m_conns = NULL;
        }

        if (server->m_listen_fd >= 0)
        {
            kq_del_read(loop->m_kq, server->m_listen_fd);
            close(server->m_listen_fd);
        }

        if (server->m_path && server->m_path_len > 0)
        {
            unlink(server->m_path);
            g_deallocate_array(loop->m_allocator, server->m_path);
            server->m_path = NULL;
        }

        uint_t last = loop->m_servers_count - 1;
        memset(server, 0, sizeof(*server));
        if (idx != last)
        {
            loop->m_servers[idx] = loop->m_servers[last];
            struct us_server* M  = &loop->m_servers[idx];
            kq_del_read(loop->m_kq, M->m_listen_fd);
            kq_add_read(loop->m_kq, M->m_listen_fd, (void*)M);
        }
        memset(&loop->m_servers[last], 0, sizeof(struct us_server));
        loop->m_servers_count--;
        return 0;
    }

    void us_loop_destroy(struct us_loop* loop)
    {
        if (!loop)
            return;

        while (loop->m_servers_count > 0)
        {
            struct us_server* server = &loop->m_servers[loop->m_servers_count - 1];
            if (server->m_active)
            {
                us_server_close(loop, server->m_server_id);
            }
            else
            {
                loop->m_servers_count--;
            }
        }

        if (loop->m_servers)
        {
            g_deallocate_array(loop->m_allocator, loop->m_servers);
        }

        if (loop->m_kq >= 0)
        {
            close(loop->m_kq);
        }

        g_deallocate(loop->m_allocator, loop);
    }

    i32 us_loop_run(struct us_loop* loop, i32 timeout_ms)
    {
        if (!loop)
        {
            errno = 22;
            return -1;
        }

        struct kevent   evlist[US_MAX_KEVENT_BATCH];
        struct timespec ts, *tsp = NULL;
        if (timeout_ms >= 0)
        {
            ts.tv_sec  = timeout_ms / 1000;
            ts.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
            tsp        = &ts;
        }

        i32 nev = kevent(loop->m_kq, NULL, 0, evlist, US_MAX_KEVENT_BATCH, tsp);
        if (nev < 0)
        {
            if (errno == 4)
                return 0;
            return -1;
        }

        for (i32 i = 0; i < nev; ++i)
        {
            struct kevent* ev    = &evlist[i];
            void*          udata = ev->udata;
            if (!udata)
                continue;
            i32* kindp = (i32*)udata;
            i32  kind  = *kindp;
            if (kind == US_KIND_SERVER)
            {
                us_server* server = (us_server*)udata;
                if (ev->filter == EVFILT_READ)
                {
                    accept_new_clients(loop, server);
                }
            }
            else if (kind == US_KIND_CONN)
            {
                us_conn*   connection = (us_conn*)udata;
                us_server* server     = connection->m_server;
                i32        need_close = 0;
                if ((ev->flags & EV_EOF) != 0)
                {
                    need_close = 1;
                }
                else if (ev->filter == EVFILT_READ)
                {
                    if (read_from_client(loop, connection) < 0)
                        need_close = 1;
                }
                if (need_close)
                {
                    uint_t connection_idx = (uint_t)-1;
                    for (uint_t k = 0; k < server->m_conns_count; ++k)
                    {
                        if (&server->m_conns[k] == connection)
                        {
                            connection_idx = k;
                            break;
                        }
                    }
                    if (connection_idx != (uint_t)-1)
                    {
                        close_conn(loop, server, connection_idx);
                    }
                    else
                    {
                        kq_del_read(loop->m_kq, connection->m_fd);
                        close(connection->m_fd);
                        if (connection->m_buf)
                        {
                            g_deallocate_array(loop->m_allocator, connection->m_buf);
                        }
                    }
                }
            }
            else
            {
                // ignore
            }
        }
        return 0;
    }

    ssize_t us_client_send(i32 client_fd, const void* data, uint_t len)
    {
        if (client_fd < 0 || !data)
        {
            errno = 22;
            return -1;
        }
        for (;;)
        {
            ssize_t n = send(client_fd, data, len, 0);
            if (n >= 0)
                return n;
            if (errno == 4)
                continue; /* EINTR */
            if (errno == 35 /* EAGAIN */ || errno == 35 /* EWOULDBLOCK alias */)
                return 0;
            return -1;
        }
    }

    i32 us_broadcast(struct us_loop* loop, i32 server_id, const void* data, uint_t len)
    {
        if (!loop || !data)
        {
            errno = 22;
            return -1;
        }
        struct us_server* server = find_server_by_id(loop, server_id, NULL);
        if (!server)
        {
            errno = 2;
            return -1;
        }
        i32 ok = 0;
        for (uint_t i = 0; i < server->m_conns_count; ++i)
        {
            ssize_t n = us_client_send(server->m_conns[i].m_fd, data, len);
            if (n >= 0)
                ok = 1;
        }
        return ok ? 0 : -1;
    }

    i32 us_client_close(us_loop* loop, i32 server_id, i32 client_fd)
    {
        if (!loop)
        {
            errno = 22;
            return -1;
        }
        us_server* server = find_server_by_id(loop, server_id, NULL);
        if (!server)
        {
            errno = 2;
            return -1;
        }
        for (uint_t i = 0; i < server->m_conns_count; ++i)
        {
            if (server->m_conns[i].m_fd == client_fd)
            {
                close_conn(loop, server, i);
                return 0;
            }
        }
        errno = 2;
        return -1;
    }

}  // namespace ncore
