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
#include "ccore/c_allocator.h"

#include "clibuv/uv.h"
#include "cmmio/c_mmio.h"
#include "cmmio/c_mmmq.h"

#include "cconartist/config.h"
#include "cconartist/conman.h"
#include "cconartist/packet_pool.h"
#include "cconartist/udp_send_pool.h"
#include "cconartist/tcp_write_pool.h"
#include "cconartist/decoder_interface.h"
#include "cconartist/decoder_plugins.h"
#include "cconartist/stream_manager.h"
#include "cconartist/stream_request.h"

#define INITIAL_CONN_CAPACITY 128

namespace ncore
{
    namespace nconartist
    {
        struct servers_t;

        struct server_tcp_t
        {
            servers_t           *m_owner;
            config_tcp_server_t *m_config;
            uv_tcp_t            *m_uv_tcp;
            nmmmq::handle_t     *m_stream;
        };

        struct server_udp_t
        {
            servers_t           *m_owner;
            config_udp_server_t *m_config;
            uv_udp_t            *m_uv_udp;
            nmmmq::handle_t     *m_stream;
        };

        struct server_discovery_t
        {
            servers_t *m_owner;
            uv_udp_t  *m_udp_server;
            u16        m_discovery_port;
            uv_buf_t   m_response_buffer;
            char      *m_response_data;
        };

        struct servers_t
        {
            uv_loop_t            *m_loop;
            server_discovery_t   *m_discovery_server;
            i32                   m_tcp_server_count;
            i32                   m_tcp_server_capacity;
            server_tcp_t         *m_tcp_servers;
            i32                   m_udp_server_count;
            i32                   m_udp_server_capacity;
            server_udp_t         *m_udp_servers;
            connection_manager_t *m_conn_mgr;
            uv_udp_t              m_udp_send_handle;  // UDP handle for sending packets
            udp_send_pool_t      *m_udp_send_pool;
            tcp_write_pool_t     *m_tcp_write_pool;
        };

        servers_t *create_servers(alloc_t *allocator, uv_loop_t *loop, int max_tcp_servers, int max_udp_servers, int max_connections)
        {
            ncore::connection_manager_t *conn_mgr = g_allocate_and_clear<connection_manager_t>(allocator);
            connection_manager_init(conn_mgr, INITIAL_CONN_CAPACITY);

            ncore::nconartist::servers_t *servers = g_allocate_and_clear<servers_t>(allocator);
            servers->m_loop                       = loop;      // Event loop
            servers->m_conn_mgr                   = conn_mgr;  // Connection manager
            servers->m_udp_send_pool              = send_pool_create(allocator, 512);
            servers->m_tcp_write_pool             = write_pool_create(allocator, 512);

            servers->m_tcp_server_capacity = max_tcp_servers;
            servers->m_tcp_servers         = g_allocate_array_and_clear<server_tcp_t>(allocator, max_tcp_servers);
            servers->m_tcp_server_count    = 0;

            servers->m_udp_server_capacity = max_udp_servers;
            servers->m_udp_servers         = g_allocate_array_and_clear<server_udp_t>(allocator, max_udp_servers);
            servers->m_udp_server_count    = 0;

            // Initialize UDP send handle, this can be used to send UDP packets to any address
            uv_udp_init(loop, &servers->m_udp_send_handle);

            return servers;
        }

        void destroy_servers(alloc_t *allocator, servers_t *servers)
        {
            connection_manager_destroy(servers->m_conn_mgr);
            allocator->deallocate(servers->m_conn_mgr);

            send_pool_destroy(servers->m_udp_send_pool);
            write_pool_destroy(servers->m_tcp_write_pool);

            allocator->deallocate(servers->m_tcp_servers);
            allocator->deallocate(servers->m_udp_servers);
            allocator->deallocate(servers);
        }

        server_tcp_t *create_tcp_server(servers_t *con_servers, config_tcp_server_t *config)
        {
            server_tcp_t *server = &con_servers->m_tcp_servers[con_servers->m_tcp_server_count];
            server->m_owner      = con_servers;
            server->m_config     = config;
            server->m_uv_tcp     = nullptr;
            con_servers->m_tcp_server_count++;
            return server;
        }

        server_udp_t *create_udp_server(servers_t *con_servers, config_udp_server_t *config)
        {
            server_udp_t *server = &con_servers->m_udp_servers[con_servers->m_udp_server_count];
            server->m_owner      = con_servers;
            server->m_config     = config;
            server->m_uv_udp     = nullptr;
            con_servers->m_udp_server_count++;
            return server;
        }

        server_discovery_t *create_udp_discovery_server(servers_t *con_servers, u16 discovery_port)
        {
            server_discovery_t *server = con_servers->m_discovery_server;
            server->m_owner            = con_servers;
            server->m_discovery_port   = discovery_port;
            server->m_udp_server       = nullptr;
            server->m_response_buffer  = {nullptr, 0};
            server->m_response_data    = nullptr;
            return server;
        }

        void on_new_tcp_connection(uv_stream_t *server, int status);

        void start_tcp_server(server_tcp_t *con_server)
        {
            servers_t *con_servers = con_server->m_owner;
            con_server->m_uv_tcp   = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
            uv_tcp_init(con_servers->m_loop, con_server->m_uv_tcp);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", con_server->m_config->m_port, &addr);  // Bind to all interfaces
            uv_tcp_bind(con_server->m_uv_tcp, (const struct sockaddr *)&addr, 0);
            con_server->m_uv_tcp->data = con_server;
            uv_listen((uv_stream_t *)con_server->m_uv_tcp, 128, on_new_tcp_connection);
        }

        void on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

        void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        void start_udp_server(server_udp_t *con_server)
        {
            servers_t *con_servers = con_server->m_owner;
            uv_udp_t  *udp         = (uv_udp_t *)malloc(sizeof(uv_udp_t));
            con_server->m_uv_udp   = udp;
            uv_udp_init(con_servers->m_loop, udp);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", con_server->m_config->m_port, &addr);
            uv_udp_bind(udp, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR);
            udp->data = con_server;
            uv_udp_recv_start(udp, alloc_buffer, on_udp_recv);
        }

        void on_udp_discovery_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

        void start_udp_discovery_server(server_discovery_t *con_server)
        {
            servers_t *con_servers   = con_server->m_owner;
            uv_udp_t  *udp           = (uv_udp_t *)malloc(sizeof(uv_udp_t));
            con_server->m_udp_server = udp;
            uv_udp_init(con_servers->m_loop, udp);
            struct sockaddr_in addr;
            uv_ip4_addr("0.0.0.0", con_server->m_discovery_port, &addr);
            uv_udp_bind(udp, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR);
            udp->data = con_server;
            uv_udp_recv_start(udp, alloc_buffer, on_udp_discovery_recv);
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
            server_tcp_t      *ctx  = info->m_server;
            if (nread > 0)
            {
                info->m_last_active = uv_hrtime();
                nmmmq::publish(ctx->m_stream, buf->base, (u32)nread);
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

            uv_tcp_t *uv_tcp_client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
            uv_tcp_init(server->loop, uv_tcp_client);
            if (uv_accept(server, (uv_stream_t *)uv_tcp_client) == 0)
            {
                server_tcp_t *con_server = (server_tcp_t *)server->data;

                struct sockaddr_storage addr;
                int                     len = sizeof(addr);
                uv_tcp_getpeername(uv_tcp_client, (struct sockaddr *)&addr, &len);
                sockaddr const *saddr = (struct sockaddr *)&addr;

                sockaddr_in const *addrin      = (struct sockaddr_in *)&addr;
                int                remote_port = ntohs(addrin->sin_port);

                struct sockaddr_storage local;
                len = sizeof(local);
                uv_tcp_getsockname(uv_tcp_client, (struct sockaddr *)&local, &len);
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
                info->set_udp();
                info->set_connected();

                info = connection_manager_commit(con_server->m_owner->m_conn_mgr, info);
                memcpy(&info->m_remote_addr, saddr, sizeof(struct sockaddr_storage));

                info->m_server      = con_server;
                info->m_handle      = uv_tcp_client;
                uv_tcp_client->data = con_server;
                uv_read_start((uv_stream_t *)uv_tcp_client, alloc_buffer, on_tcp_read);
            }
            else
            {
                uv_close((uv_handle_t *)uv_tcp_client, on_tcp_close);
            }
        }

        void on_udp_send_done(uv_udp_send_t *req, int status)
        {
            udp_send_pool_t *send_pool = (udp_send_pool_t *)req->data;
            send_pool_release(send_pool, req);
        }

        void on_udp_discovery_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
        {
            // Handle UDP discovery packet reception
            // Validate the discovery packet and send a response back
            // The response should contain the server's IP address, TCP port, and UDP port
            if (nread > 0 && addr)
            {
                server_discovery_t *con_server  = (server_discovery_t *)handle->data;
                servers_t          *con_servers = con_server->m_owner;

                // Validate the discovery packet (implementation-specific)

                // Prepare response packet
                // For simplicity, we assume the response is already prepared in con_server->m_response_data
                uv_buf_t response_buf;
                response_buf.base = con_server->m_response_data;
                response_buf.len  = strlen(con_server->m_response_data);

                uv_udp_send_t *send_req = send_pool_acquire(con_servers->m_udp_send_pool);
                send_req->handle        = handle;
                send_req->data          = con_servers->m_udp_send_pool;  // For release callback
                uv_udp_send(send_req, &con_servers->m_udp_send_handle, &response_buf, 1, addr, on_udp_send_done);
            }
            if (buf->base)
                free(buf->base);
        }

        void on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
        {
            if (nread > 0 && addr)
            {
                server_udp_t *server  = (server_udp_t *)handle->data;
                nmmmq::publish(server->m_stream, buf->base, (u32)nread);
            }
            if (buf->base)
                free(buf->base);
        }

        void on_prepare(uv_prepare_t *handle)
        {
            // Called once per loop iteration, before the I/O poll
            (void)handle;
            // Do lightweight work, metrics, scheduling, etc.
            // Keep it fast to avoid delaying I/O polling
        }

        void on_check(uv_check_t *handle)
        {
            // Called once per loop iteration, after the I/O poll
            (void)handle;
            // Good for tasks that depend on completed I/O
        }

    }  // namespace nconartist
}  // namespace ncore

using namespace ncore;

class malloc_based_allocator : public alloc_t
{
public:
    malloc_based_allocator() {}
    virtual void *v_allocate(u32 size, u32 align) override final { return malloc(size); }
    virtual void  v_deallocate(void *ptr) override final { free(ptr); }
};

// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------

int main()
{
    malloc_based_allocator mba;
    alloc_t               *allocator = &mba;

    ncore::config_main_t *config = ncore::g_load_config(allocator);
    if (!config)
    {
        printf("Failed to load configuration.\n");
        return -1;
    }

    // Create the event loop
    uv_loop_t *loop = uv_default_loop();

    // Create 'servers' context
    ncore::nconartist::servers_t *servers = ncore::nconartist::create_servers(allocator, loop, config->m_num_tcp_servers, config->m_num_udp_servers, INITIAL_CONN_CAPACITY);

    printf("Starting servers...\n");
    for (int i = 0; i < config->m_num_tcp_servers; i++)
    {
        ncore::nconartist::server_tcp_t *server = ncore::nconartist::create_tcp_server(servers, &config->m_tcp_servers[i]);
        printf("Starting TCP server on port %d\n", server->m_config->m_port);
        ncore::nconartist::start_tcp_server(server);
    }
    for (int i = 0; i < config->m_num_udp_servers; i++)
    {
        ncore::nconartist::server_udp_t *server = ncore::nconartist::create_udp_server(servers, &config->m_udp_servers[i]);
        printf("Starting UDP server on port %d\n", server->m_config->m_port);
        ncore::nconartist::start_udp_server(server);
    }

    // Initialize and start UDP discovery server
    ncore::nconartist::server_discovery_t *discovery_server = ncore::nconartist::create_udp_discovery_server(servers, config->m_discovery_port);
    printf("Starting UDP discovery server on port %d\n", discovery_server->m_discovery_port);
    ncore::nconartist::start_udp_discovery_server(discovery_server);

    // TODO: Have a function poll a (shared) memory mapped file for messages to send to TCP and UDP clients

    // TODO: Use ctui to create a simple terminal UI for monitoring connections showing basic info and stats

    // Initialize and start the prepare handle
    uv_prepare_t prepare;
    uv_prepare_init(loop, &prepare);
    uv_prepare_start(&prepare, ncore::nconartist::on_prepare);

    // Initialize and start the check handle (optional)
    uv_check_t check;
    uv_check_init(loop, &check);
    uv_check_start(&check, ncore::nconartist::on_check);

    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    ncore::nconartist::destroy_servers(allocator, servers);

    uv_prepare_stop(&prepare);
    uv_check_stop(&check);
    uv_loop_close(loop);

    return 0;
}
