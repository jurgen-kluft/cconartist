#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "ccore/c_target.h"
#include "clibuv/uv.h"

#include "cconartist/channel.h"
#include "cconartist/conman.h"
#include "cconartist/packet_pool.h"
#include "cconartist/udp_send_pool.h"
#include "cconartist/tcp_write_pool.h"
#include "cconartist/decoder_plugins.h"
#include "cconartist/streamman.h"

#define INITIAL_CONN_CAPACITY 128
#define PACKET_POOL_SIZE      1024

namespace ncore
{
    namespace nconartist
    {
        const int cTcpServerType = 0;
        const int cUdpServerType = 1;

        struct server_config_t
        {
            int  m_port;      // Port number
            int  m_type;      // cTcpServerType or cUdpServerType
            char m_name[64];  // Server/Plugin name
        };

        struct servers_t;

        struct server_t
        {
            servers_t        *m_owner;
            server_config_t  *m_config;
            uv_tcp_t         *m_tcp_server;
            uv_udp_t         *m_udp_server;
            stream_context_t *m_stream_ctx;
        };

        struct servers_t
        {
            uv_loop_t            *m_loop;
            channel_t            *m_channel_packets_out;  // Libuv → UI
            channel_t            *m_channel_packets_in;   // UI → Libuv
            int                   m_server_count;
            server_t             *m_servers;
            connection_manager_t *m_conn_mgr;
            packet_pool_t         m_packet_pool;
            uv_async_t            m_async_send;       // async handle for sending packets
            uv_udp_t              m_udp_send_handle;  // UDP handle for sending packets
            udp_send_pool_t      *m_udp_send_pool;
            tcp_write_pool_t     *m_tcp_write_pool;
            nplugins::registry_t *m_decoder_registry;
        };

        server_t *create_server(servers_t *con_servers, server_config_t *config)
        {
            server_t *server     = &con_servers->m_servers[con_servers->m_server_count];
            server->m_owner      = con_servers;
            server->m_config     = config;
            server->m_tcp_server = nullptr;
            server->m_udp_server = nullptr;
            server->m_stream_ctx = nullptr;
            con_servers->m_server_count++;
            return server;
        }

        void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
        {
            buf->base = (char *)malloc(suggested_size);
            buf->len  = suggested_size;
        }

        void after_write(uv_write_t *req, int status)
        {
            free(req->data);
            free(req);
        }

        void on_tcp_close(uv_handle_t *handle) { free(handle); }

        void on_tcp_write_done(uv_write_t *req, int status)
        {
            if (req->data)
                free(req->data);
            servers_t *con_servers = (servers_t *)req->handle->data;
            write_pool_release(con_servers->m_tcp_write_pool, req);
        }

        void on_tcp_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
        {
            connection_info_t *info = (connection_info_t *)client->data;
            server_t          *ctx  = info->m_server;
            if (nread > 0)
            {
                packet_t *pkt = packet_acquire(ctx->m_owner->m_packet_pool);
                if (pkt)
                {
                    pkt->m_conn = info;
                    pkt->m_size = nread;
                    memcpy(pkt->m_data, buf->base, nread);
                    channel_push(ctx->m_owner->m_channel_packets_out, pkt);

                    info->m_last_active = uv_hrtime();

                    printf("Received TCP packet from %s:%d, size: %zu\n", info->m_remote_ip, info->m_remote_port, pkt->m_size);
                }
                else
                {
                    // Failed to acquire packet, consider logging or handling this case
                }
            }
            else if (nread == UV_EOF)
            {
                connection_manager_mark_disconnected(ctx->m_owner->m_conn_mgr, info);
                uv_close((uv_handle_t *)client, on_tcp_close);
            }
            if (buf->base)
                free(buf->base);
        }

        void on_new_tcp_connection(uv_stream_t *server, int status)
        {
            if (status < 0)
                return;
            uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
            uv_tcp_init(server->loop, client);
            if (uv_accept(server, (uv_stream_t *)client) == 0)
            {
                server_t *con_server = (server_t *)server->data;

                struct sockaddr_storage addr;
                int                     len = sizeof(addr);
                uv_tcp_getpeername(client, (struct sockaddr *)&addr, &len);
                sockaddr const *saddr = (struct sockaddr *)&addr;

                sockaddr_in const *addrin      = (struct sockaddr_in *)&addr;
                int                remote_port = ntohs(addrin->sin_port);

                struct sockaddr_storage local;
                len = sizeof(local);
                uv_tcp_getsockname(client, (struct sockaddr *)&local, &len);
                int local_port = ntohs(((struct sockaddr_in *)&local)->sin_port);

                connection_info_t *info = connection_manager_alloc(con_server->m_owner->m_conn_mgr);
                if (saddr->sa_family == AF_INET)
                {
                    // Handle IPv4 address extraction
                    uint32_t const *ipv4 = (uint32_t const *)&((struct sockaddr_in *)saddr)->sin_addr.s_addr;
                    info->m_remote_ip[0] = ipv4[0];
                    info->m_remote_ip[1] = 0;
                    info->m_remote_ip[2] = 0;
                    info->m_remote_ip[3] = 0;
                }
                else if (saddr->sa_family == AF_INET6)
                {
                    // Handle IPv6 address extraction
                    uint32_t const *ipv6 = (uint32_t const *)&((struct sockaddr_in6 *)saddr)->sin6_addr.s6_addr[0];
                    info->m_remote_ip[0] = ipv6[0];
                    info->m_remote_ip[1] = ipv6[1];
                    info->m_remote_ip[2] = ipv6[2];
                    info->m_remote_ip[3] = ipv6[3];
                }
                info->m_remote_port = remote_port;
                info->m_local_port  = local_port;
                info->m_type        = CONN_UDP;

                info = connection_manager_commit(con_server->m_owner->m_conn_mgr, info);
                memcpy(&info->m_remote_addr, saddr, sizeof(struct sockaddr_storage));

                info->m_server = con_server;
                info->m_handle = client;
                client->data   = con_server;
                uv_read_start((uv_stream_t *)client, alloc_buffer, on_tcp_read);
            }
            else
            {
                uv_close((uv_handle_t *)client, on_tcp_close);
            }
        }

        void start_tcp_server(server_t *con_server)
        {
            servers_t *con_servers   = con_server->m_owner;
            con_server->m_tcp_server = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
            uv_tcp_init(con_servers->m_loop, con_server->m_tcp_server);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", con_server->m_config->m_port, &addr);  // Bind to all interfaces
            uv_tcp_bind(con_server->m_tcp_server, (const struct sockaddr *)&addr, 0);
            con_server->m_tcp_server->data = con_server;
            uv_listen((uv_stream_t *)con_server->m_tcp_server, 128, on_new_tcp_connection);
        }

        void on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
        {
            if (nread > 0 && addr)
            {
                server_t  *con_server  = (server_t *)handle->data;
                servers_t *con_servers = con_server->m_owner;
                packet_t  *pkt         = packet_acquire(con_servers->m_packet_pool);
                if (pkt)
                {
                    connection_info_t *info = connection_manager_alloc(con_servers->m_conn_mgr);
                    if (addr->sa_family == AF_INET)
                    {
                        // Handle IPv4 address extraction
                        uint32_t const *ipv4 = (uint32_t const *)&((struct sockaddr_in *)addr)->sin_addr.s_addr;
                        info->m_remote_ip[0] = ipv4[0];
                        info->m_remote_ip[1] = 0;
                        info->m_remote_ip[2] = 0;
                        info->m_remote_ip[3] = 0;
                    }
                    else if (addr->sa_family == AF_INET6)
                    {
                        // Handle IPv6 address extraction
                        uint32_t const *ipv6 = (uint32_t const *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr[0];
                        info->m_remote_ip[0] = ipv6[0];
                        info->m_remote_ip[1] = ipv6[1];
                        info->m_remote_ip[2] = ipv6[2];
                        info->m_remote_ip[3] = ipv6[3];
                    }
                    info->m_remote_port = ntohs(((struct sockaddr_in *)addr)->sin_port);
                    info->m_type        = CONN_UDP;

                    info = connection_manager_commit(con_servers->m_conn_mgr, info);
                    memcpy(&info->m_remote_addr, addr, sizeof(struct sockaddr_storage));

                    pkt->m_conn = info;
                    pkt->m_size = nread;
                    memcpy(pkt->m_data, buf->base, nread);
                    channel_push(con_servers->m_channel_packets_out, pkt);

                    // printf("Received UDP packet from %s:%d, size: %zu\n", ip, port, pkt->m_size);
                }
            }
            if (buf->base)
                free(buf->base);
        }

        void start_udp_server(server_t *con_server)
        {
            servers_t *con_servers   = con_server->m_owner;
            uv_udp_t  *udp           = (uv_udp_t *)malloc(sizeof(uv_udp_t));
            con_server->m_udp_server = udp;
            uv_udp_init(con_servers->m_loop, udp);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", con_server->m_config->m_port, &addr);
            uv_udp_bind(udp, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR);
            udp->data = con_server;
            uv_udp_recv_start(udp, alloc_buffer, on_udp_recv);
        }

        void start_server(server_t *con_server)
        {
            if (con_server->m_config->m_type == 0)
            {
                printf("Starting TCP server on port %d\n", con_server->m_config->m_port);
                start_tcp_server(con_server);
            }
            else if (con_server->m_config->m_type == 1)
            {
                printf("Starting UDP server on port %d\n", con_server->m_config->m_port);
                start_udp_server(con_server);
            }
        }

        // Async callback for sending packets

        void on_udp_send_done(uv_udp_send_t *req, int status)
        {
            server_t *con_server = (server_t *)req->handle->data;
            send_pool_release(con_server->m_owner->m_udp_send_pool, req);
        }

        void on_async_send(uv_async_t *handle)
        {
            servers_t *con_servers = (servers_t *)handle->data;
            while (1)
            {
                packet_t *pkt = (packet_t *)channel_pop(con_servers->m_channel_packets_in);
                if (!pkt)
                    break;

                if (pkt->m_conn && pkt->m_conn->m_state == STATE_CONNECTED)
                {
                    if (pkt->m_conn->m_type == CONN_TCP)
                    {
                        if (pkt->m_conn->m_handle)
                        {
                            uv_write_t *req = write_pool_acquire(con_servers->m_tcp_write_pool);
                            if (req)
                            {
                                // TODO also reuse uv_buf_t from a pool
                                uv_buf_t buf = uv_buf_init((char *)malloc(pkt->m_size), pkt->m_size);
                                memcpy(buf.base, pkt->m_data, pkt->m_size);
                                req->data = buf.base;
                                uv_write(req, (uv_stream_t *)pkt->m_conn->m_handle, &buf, 1, on_tcp_write_done);
                            }
                            else
                            {
                                fprintf(stderr, "Send pool exhausted for TCP!\n");
                            }
                        }
                    }
                    else if (pkt->m_conn->m_type == CONN_UDP)
                    {
                        // TODO also reuse uv_buf_t from a pool
                        uv_buf_t       buf      = uv_buf_init(pkt->m_data, pkt->m_size);
                        uv_udp_send_t *send_req = send_pool_acquire(con_servers->m_udp_send_pool);
                        if (send_req)
                        {
                            uv_udp_send(send_req, &con_servers->m_udp_send_handle, &buf, 1, (const struct sockaddr *)&pkt->m_conn->m_remote_addr, on_udp_send_done);
                        }
                        else
                        {
                            fprintf(stderr, "Send pool exhausted for UDP!\n");
                        }
                    }
                }
                packet_release(con_servers->m_packet_pool, pkt);
            }
        }

        void ui_thread(void *arg)
        {
            servers_t *con_servers = (servers_t *)arg;
            while (1)
            {
                packet_t *pkt = (packet_t *)channel_pop(con_servers->m_channel_packets_out);
                printf("[UI] Received packet from %s:%d size=%zu\n", pkt->m_conn->m_remote_ip, pkt->m_conn->m_remote_port, pkt->m_size);

                // Example: send response back
                packet_t *out_pkt = packet_acquire(con_servers->m_packet_pool);
                if (out_pkt)
                {
                    out_pkt->m_conn = pkt->m_conn;
                    const char *msg = "Hello from UI!";
                    out_pkt->m_size = strlen(msg);
                    memcpy(out_pkt->m_data, msg, out_pkt->m_size);
                    channel_push(con_servers->m_channel_packets_in, out_pkt);
                    uv_async_send(&con_servers->m_async_send);  // Signal event loop immediately
                }
                packet_release(con_servers->m_packet_pool, pkt);
            }
        }
    }  // namespace nconartist
}  // namespace ncore

using namespace ncore;

static ncore::nconartist::server_config_t s_server_config[] = {
  {31330, ncore::nconartist::cTcpServerType, "GeekOpen"},         // GeekOpen TCP server
  {31372, ncore::nconartist::cTcpServerType, "SensorPacket"},     // SensorPacket TCP server
  {31373, ncore::nconartist::cTcpServerType, "ImagePacket"},      // ImagePacket TCP server
  {31370, ncore::nconartist::cUdpServerType, "SensorPacket"},     // SensorPacket UDP server
  {31371, ncore::nconartist::cUdpServerType, "DiscoveryPacket"},  // DiscoveryPacket UDP server
};

int main()
{
    uv_loop_t       *loop = uv_default_loop();
    ncore::channel_t channel_packets_out, channel_packets_in;
    channel_init(&channel_packets_out, 1024);
    channel_init(&channel_packets_in, 1024);
    ncore::connection_manager_t conn_mgr;
    connection_manager_init(&conn_mgr, INITIAL_CONN_CAPACITY);

    ncore::nconartist::servers_t ctx;                  // Manager of all servers
    ctx.m_loop                = loop;                  // Event loop
    ctx.m_channel_packets_out = &channel_packets_out;  // Libuv → UI
    ctx.m_channel_packets_in  = &channel_packets_in;   // UI → Libuv
    ctx.m_conn_mgr            = &conn_mgr;             // Connection manager
    packet_pool_init(PACKET_POOL_SIZE, ctx.m_packet_pool);
    ctx.m_udp_send_pool    = (ncore::udp_send_pool_t *)malloc(sizeof(udp_send_pool_t));
    ctx.m_tcp_write_pool   = (ncore::tcp_write_pool_t *)malloc(sizeof(tcp_write_pool_t));
    ctx.m_decoder_registry = ncore::nplugins::create_registry("./plugins", 64, loop);

    send_pool_init(ctx.m_udp_send_pool);
    write_pool_init(ctx.m_tcp_write_pool);

    uv_async_init(loop, &ctx.m_async_send, ncore::nconartist::on_async_send);
    ctx.m_async_send.data = &ctx;

    uv_udp_init(loop, &ctx.m_udp_send_handle);

    uv_thread_t ui;
    uv_thread_create(&ui, ncore::nconartist::ui_thread, &ctx);

    printf("Starting servers...\n");
    for (int i = 0; i < (int)DARRAYSIZE(s_server_config); i++)
    {
        ncore::nconartist::server_t *server = ncore::nconartist::create_server(&ctx, &s_server_config[i]);
        ncore::nconartist::start_server(server);
    }

    // TODO: Use ctui to create a simple terminal UI for monitoring connections showing basic info and stats

    uv_run(loop, UV_RUN_DEFAULT);

    connection_manager_destroy(&conn_mgr);
    channel_destroy(&channel_packets_out);
    channel_destroy(&channel_packets_in);
    packet_pool_release(ctx.m_packet_pool);
    return 0;
}
