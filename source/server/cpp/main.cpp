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
#include "cconartist/uv_buf_allocator.h"
#include "cconartist/uv_udp_send_pool.h"
#include "cconartist/uv_tcp_write_pool.h"
#include "cconartist/uv_tcp_pool.h"
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
            byte      *m_response_data;
        };

        struct servers_t
        {
            uv_loop_t           *m_loop;
            server_discovery_t  *m_discovery_server;
            i32                  m_tcp_server_count;
            i32                  m_tcp_server_capacity;
            server_tcp_t       **m_tcp_servers;
            i32                  m_udp_server_count;
            i32                  m_udp_server_capacity;
            server_udp_t       **m_udp_servers;
            tcp_con_mgr_t       *m_conn_mgr;
            uv_buf_alloc_t      *m_buf_allocator;
            uv_udp_t             m_udp_send_handle;  // UDP handle for sending packets
            uv_udp_send_pool_t  *m_udp_send_pool;
            uv_tcp_pool_t       *m_tcp_pool;
            uv_tcp_write_pool_t *m_tcp_write_pool;
        };

        server_tcp_t       *create_tcp_server(alloc_t *allocator, servers_t *con_servers, config_tcp_server_t *config);
        server_udp_t       *create_udp_server(alloc_t *allocator, servers_t *con_servers, config_udp_server_t *config);
        server_discovery_t *create_udp_discovery_server(alloc_t *allocator, servers_t *con_servers, u16 discovery_port);
        void                start_tcp_server(server_tcp_t *server);
        void                start_udp_server(server_udp_t *server);
        void                start_udp_discovery_server(server_discovery_t *server);

        servers_t *create_servers(alloc_t *allocator, uv_loop_t *loop, ncore::config_main_t *config, int max_connections)
        {
            ncore::nconartist::servers_t *servers = g_allocate_and_clear<servers_t>(allocator);
            servers->m_loop                       = loop;  // Event loop
            servers->m_conn_mgr                   = tcp_con_mgr_create(allocator, INITIAL_CONN_CAPACITY);
            servers->m_buf_allocator              = uv_buf_alloc_create(allocator, 4 * cMB, 64 * cMB);
            servers->m_udp_send_pool              = uv_udp_send_pool_create(allocator, 512);
            servers->m_tcp_write_pool             = uv_tcp_write_pool_create(allocator, max_connections);
            servers->m_tcp_pool                   = uv_tcp_pool_create(allocator, max_connections);

            servers->m_tcp_server_capacity = config->m_num_tcp_servers;
            servers->m_tcp_servers         = g_allocate_array_and_clear<server_tcp_t *>(allocator, config->m_num_tcp_servers);
            servers->m_tcp_server_count    = 0;

            servers->m_udp_server_capacity = config->m_num_udp_servers;
            servers->m_udp_servers         = g_allocate_array_and_clear<server_udp_t *>(allocator, config->m_num_udp_servers);
            servers->m_udp_server_count    = 0;

            // Initialize UDP send handle, this can be used to send UDP packets to any address
            uv_udp_init(loop, &servers->m_udp_send_handle);

            for (int i = 0; i < config->m_num_tcp_servers; i++)
            {
                ncore::nconartist::server_tcp_t *server = create_tcp_server(allocator, servers, &config->m_tcp_servers[i]);
                if (server == nullptr)
                {
                    printf("Failed to create TCP server for %s\n", config->m_tcp_servers[i].m_server_name);
                }
                else
                {
                    servers->m_tcp_servers[i] = server;
                    servers->m_tcp_server_count++;
                }
            }
            for (int i = 0; i < config->m_num_udp_servers; i++)
            {
                ncore::nconartist::server_udp_t *server = create_udp_server(allocator, servers, &config->m_udp_servers[i]);
                if (server == nullptr)
                {
                    printf("Failed to create UDP server for %s\n", config->m_udp_servers[i].m_server_name);
                }
                else
                {
                    servers->m_udp_servers[i] = server;
                    servers->m_udp_server_count++;
                }
            }

            // Initialize and start UDP discovery server
            servers->m_discovery_server = ncore::nconartist::create_udp_discovery_server(allocator, servers, config->m_discovery_port);
            if (servers->m_discovery_server == nullptr)
            {
                printf("Failed to create UDP discovery server on port %d\n", config->m_discovery_port);
            }
            return servers;
        }

        void start_servers(alloc_t *allocator, servers_t *servers)
        {
            printf("Starting servers...\n");
            for (int i = 0; i < servers->m_tcp_server_count; i++)
            {
                ncore::nconartist::server_tcp_t *server = servers->m_tcp_servers[i];
                if (server == nullptr)
                    continue;
                printf("Starting TCP server on port %d\n", server->m_config->m_port);
                start_tcp_server(server);
            }
            for (int i = 0; i < servers->m_udp_server_count; i++)
            {
                server_udp_t *server = servers->m_udp_servers[i];
                if (server == nullptr)
                    continue;
                printf("Starting UDP server on port %d\n", server->m_config->m_port);
                start_udp_server(server);
            }

            // Initialize and start UDP discovery server
            server_discovery_t *discovery_server = servers->m_discovery_server;
            printf("Starting UDP discovery server on port %d\n", discovery_server->m_discovery_port);
            start_udp_discovery_server(discovery_server);
        }

        server_tcp_t *create_tcp_server(alloc_t *allocator, servers_t *con_servers, config_tcp_server_t *config)
        {
            server_tcp_t *server = g_allocate_and_clear<server_tcp_t>(allocator);
            server->m_owner      = con_servers;
            server->m_config     = config;
            server->m_uv_tcp     = nullptr;
            server->m_uv_tcp     = g_allocate_and_clear<uv_tcp_t>(allocator);

            // create the nmmio mmmq stream for this server
            nmmmq::config_t mmq_config(config->m_stream_config.m_index_size * cMB, config->m_stream_config.m_data_size * cMB, config->m_stream_config.m_max_consumers);
            server->m_stream = nmmmq::create_handle(allocator);
            const i32 result = nmmmq::init_producer(server->m_stream, mmq_config, config->m_stream_config.m_index_filename, config->m_stream_config.m_data_filename, config->m_stream_config.m_control_filename, config->m_stream_config.m_new_sem_name,
                                                    config->m_stream_config.m_reg_sem_name);
            if (result < 0)
            {
                nmmmq::destroy_handle(server->m_stream);
                return nullptr;
            }

            return server;
        }

        void destroy_tcp_server(alloc_t *allocator, server_tcp_t *server)
        {
            nmmmq::close_handle(server->m_stream);
            nmmmq::destroy_handle(server->m_stream);
            allocator->deallocate(server->m_uv_tcp);
        }

        server_udp_t *create_udp_server(alloc_t *allocator, servers_t *con_servers, config_udp_server_t *config)
        {
            server_udp_t *server = g_allocate_and_clear<server_udp_t>(allocator);
            server->m_owner      = con_servers;
            server->m_config     = config;
            server->m_uv_udp     = g_allocate_and_clear<uv_udp_t>(allocator);

            // create the nmmio mmmq stream for this server
            nmmmq::config_t mmq_config(config->m_stream_config.m_index_size * cMB, config->m_stream_config.m_data_size * cMB, config->m_stream_config.m_max_consumers);
            server->m_stream = nmmmq::create_handle(allocator);
            const i32 result = nmmmq::init_producer(server->m_stream, mmq_config, config->m_stream_config.m_index_filename, config->m_stream_config.m_data_filename, config->m_stream_config.m_control_filename, config->m_stream_config.m_new_sem_name,
                                                    config->m_stream_config.m_reg_sem_name);
            if (result < 0)
            {
                nmmmq::destroy_handle(server->m_stream);
                return nullptr;
            }

            return server;
        }

        void destroy_udp_server(alloc_t *allocator, server_udp_t *server)
        {
            nmmmq::close_handle(server->m_stream);
            nmmmq::destroy_handle(server->m_stream);
            allocator->deallocate(server->m_uv_udp);
        }

        server_discovery_t *create_udp_discovery_server(alloc_t *allocator, servers_t *con_servers, u16 discovery_port)
        {
            server_discovery_t *server = g_allocate_and_clear<server_discovery_t>(allocator);
            server->m_owner            = con_servers;
            server->m_discovery_port   = discovery_port;
            server->m_udp_server       = g_allocate_and_clear<uv_udp_t>(allocator);
            server->m_response_buffer  = {nullptr, 0};
            server->m_response_data    = nullptr;

            // prepare the response data buffer
            // For simplicity, we just respond with a fixed message that
            // includes our IP, TCP and UDP ports
            const char *response_template = "CONARTIST-IP=%s;SENSOR-TCP=%u;SENSOR-UDP=%u;IMAGE-TCP=%u";

            // get our LAN ip-address from libuv
            char                    ip_buffer[17] = {0};
            uv_interface_address_t *addresses     = nullptr;
            int                     count         = 0;
            if (uv_interface_addresses(&addresses, &count) == 0)
            {
                for (int i = 0; i < count; ++i)
                {
                    uv_interface_address_t &address = addresses[i];
                    if (!address.is_internal && address.address.address4.sin_family == AF_INET)
                    {
                        uv_ip4_name(&address.address.address4, ip_buffer, sizeof(ip_buffer));
                        break;
                    }
                }
                uv_free_interface_addresses(addresses, count);
            }

            // format the response data
            config_tcp_server_t *sensor_tcp = con_servers->m_tcp_server_count > 0 ? con_servers->m_tcp_servers[0]->m_config : nullptr;
            config_tcp_server_t *image_tcp  = con_servers->m_tcp_server_count > 1 ? con_servers->m_tcp_servers[1]->m_config : nullptr;
            config_udp_server_t *udp_server = con_servers->m_udp_server_count > 0 ? con_servers->m_udp_servers[0]->m_config : nullptr;

            int response_size       = snprintf(nullptr, 0, response_template, ip_buffer, sensor_tcp ? sensor_tcp->m_port : 0, image_tcp ? image_tcp->m_port : 0, udp_server ? udp_server->m_port : 0);
            server->m_response_data = g_allocate_array_and_clear<byte>(allocator, response_size + 1);
            snprintf((char *)server->m_response_data, response_size + 1, response_template, ip_buffer, sensor_tcp ? sensor_tcp->m_port : 0, image_tcp ? image_tcp->m_port : 0, udp_server ? udp_server->m_port : 0);
            server->m_response_buffer.base = (char *)server->m_response_data;
            server->m_response_buffer.len  = (size_t)response_size;
            return server;
        }

        void destroy_udp_discovery_server(alloc_t *allocator, server_discovery_t *server)
        {
            if (server->m_response_data)
            {
                allocator->deallocate(server->m_response_data);
            }
            allocator->deallocate(server);
        }

        void destroy_servers(alloc_t *allocator, servers_t *servers)
        {
            tcp_con_mgr_destroy(servers->m_conn_mgr);
            allocator->deallocate(servers->m_conn_mgr);

            uv_udp_send_pool_destroy(servers->m_udp_send_pool);
            uv_tcp_write_pool_destroy(servers->m_tcp_write_pool);

            for (i32 i = 0; i < servers->m_tcp_server_count; ++i)
            {
                destroy_tcp_server(allocator, servers->m_tcp_servers[i]);
            }
            for (i32 i = 0; i < servers->m_udp_server_count; ++i)
            {
                destroy_udp_server(allocator, servers->m_udp_servers[i]);
            }
            if (servers->m_discovery_server)
            {
                destroy_udp_discovery_server(allocator, servers->m_discovery_server);
            }

            allocator->deallocate(servers->m_tcp_servers);
            allocator->deallocate(servers->m_udp_servers);
            allocator->deallocate(servers);
        }

        static void handle_uv_result(int result, const char *message)
        {
            if (result != 0)
            {
                fprintf(stderr, message, uv_err_name(result));
            }
        }

        void on_new_tcp_connection(uv_stream_t *server, int status);

        void start_tcp_server(server_tcp_t *server)
        {
            const char *error_msg = "TCP server start error: %s\n";
            servers_t  *servers   = server->m_owner;
            handle_uv_result(uv_tcp_init(servers->m_loop, server->m_uv_tcp), error_msg);
            struct sockaddr_in addr;
            handle_uv_result(uv_ip4_addr("0.0.0.0", server->m_config->m_port, &addr), error_msg);  // Bind to all interfaces
            handle_uv_result(uv_tcp_bind(server->m_uv_tcp, (const struct sockaddr *)&addr, 0), error_msg);
            server->m_uv_tcp->data = server;
            handle_uv_result(uv_listen((uv_stream_t *)server->m_uv_tcp, 128, on_new_tcp_connection), error_msg);
        }

        void on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
        void alloc_before_udp_recv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        void start_udp_server(server_udp_t *server)
        {
            const char *error_msg = "UDP server start error: %s\n";
            servers_t  *servers   = server->m_owner;
            handle_uv_result(uv_udp_init(servers->m_loop, server->m_uv_udp), error_msg);
            struct sockaddr_in addr;
            handle_uv_result(uv_ip4_addr("0.0.0.0", server->m_config->m_port, &addr), error_msg);
            handle_uv_result(uv_udp_bind(server->m_uv_udp, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR), error_msg);
            server->m_uv_udp->data = server;
            handle_uv_result(uv_udp_recv_start(server->m_uv_udp, alloc_before_udp_recv, on_udp_recv), error_msg);
        }

        void on_udp_discovery_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
        void alloc_before_udp_discovery_recv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        void start_udp_discovery_server(server_discovery_t *server)
        {
            const char *error_msg = "UDP discovery server start error: %s\n";
            servers_t  *servers   = server->m_owner;
            handle_uv_result(uv_udp_init(servers->m_loop, server->m_udp_server), error_msg);
            struct sockaddr_in addr;
            handle_uv_result(uv_ip4_addr("0.0.0.0", server->m_discovery_port, &addr), error_msg);
            handle_uv_result(uv_udp_bind(server->m_udp_server, (const struct sockaddr *)&addr, UV_UDP_REUSEADDR), error_msg);
            server->m_udp_server->data = server;
            handle_uv_result(uv_udp_recv_start(server->m_udp_server, alloc_before_udp_discovery_recv, on_udp_discovery_recv), error_msg);
        }

        void after_write(uv_write_t *req, int status)
        {
            free(req->data);
            free(req);
        }

        void on_tcp_close(uv_handle_t *handle)
        {
            tcp_con_t *tcp_con = (tcp_con_t *)handle->data;
            tcp_con->set_disconnected();
            tcp_con_free(tcp_con->m_server->m_owner->m_conn_mgr, tcp_con);
        }

        void on_tcp_write_done(uv_write_t *req, int status)
        {
            if (req->data)
                free(req->data);
            servers_t *con_servers = (servers_t *)req->handle->data;
            uv_tcp_write_release(con_servers->m_tcp_write_pool, req);
        }

        void alloc_before_tcp_read(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
        {
            tcp_con_t *tcp_con = (tcp_con_t *)handle->data;
            buf->base          = (char *)uv_buf_alloc(tcp_con->m_server->m_owner->m_buf_allocator, suggested_size);
            buf->len           = suggested_size;
        }

        void on_tcp_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
        {
            tcp_con_t    *tcp_con    = (tcp_con_t *)client->data;
            server_tcp_t *tcp_server = tcp_con->m_server;
            if (nread > 0)
            {
                tcp_con->m_last_active = uv_hrtime();
                nmmmq::publish(tcp_server->m_stream, buf->base, (u32)nread);
            }
            else if (nread == UV_EOF)
            {
                tcp_con->set_disconnected();
                uv_close((uv_handle_t *)client, on_tcp_close);
            }
            byte *data = (byte *)buf->base;
            uv_buf_release(tcp_con->m_server->m_owner->m_buf_allocator, data);
        }

        void on_new_tcp_connection(uv_stream_t *server, int status)
        {
            if (status < 0)
                return;

            server_tcp_t *con_server = (server_tcp_t *)server->data;

            tcp_con_t *tcp_con       = tcp_con_alloc(con_server->m_owner->m_conn_mgr);
            uv_tcp_t  *uv_tcp_client = uv_tcp_acquire(con_server->m_owner->m_tcp_pool);
            uv_tcp_init(server->loop, uv_tcp_client);
            uv_tcp_client->data = tcp_con;

            if (uv_accept(server, (uv_stream_t *)uv_tcp_client) == 0)
            {
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

                if (saddr->sa_family == AF_INET)
                {
                    // Handle IPv4 address extraction
                    uint32_t const *ipv4    = (uint32_t const *)&((struct sockaddr_in *)saddr)->sin_addr.s_addr;
                    tcp_con->m_remote_ip[0] = ipv4[0];
                    tcp_con->m_remote_ip[1] = 0;
                    tcp_con->m_remote_ip[2] = 0;
                    tcp_con->m_remote_ip[3] = 0;
                }
                else if (saddr->sa_family == AF_INET6)
                {
                    // Handle IPv6 address extraction
                    uint32_t const *ipv6    = (uint32_t const *)&((struct sockaddr_in6 *)saddr)->sin6_addr.s6_addr[0];
                    tcp_con->m_remote_ip[0] = ipv6[0];
                    tcp_con->m_remote_ip[1] = ipv6[1];
                    tcp_con->m_remote_ip[2] = ipv6[2];
                    tcp_con->m_remote_ip[3] = ipv6[3];
                }
                tcp_con->m_remote_port = remote_port;
                tcp_con->m_local_port  = local_port;
                tcp_con->set_connected();

                tcp_con->m_server = con_server;
                tcp_con->m_handle = uv_tcp_client;
                uv_read_start((uv_stream_t *)uv_tcp_client, alloc_before_tcp_read, on_tcp_read);
            }
            else
            {
                uv_close((uv_handle_t *)uv_tcp_client, on_tcp_close);
            }
        }

        void alloc_before_udp_recv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
        {
            server_udp_t *server = (server_udp_t *)handle->data;
            buf->base            = (char *)uv_buf_alloc(server->m_owner->m_buf_allocator, suggested_size);
            buf->len             = suggested_size;
        }

        void alloc_before_udp_discovery_recv(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
        {
            server_discovery_t *server = (server_discovery_t *)handle->data;
            buf->base                  = (char *)uv_buf_alloc(server->m_owner->m_buf_allocator, suggested_size);
            buf->len                   = suggested_size;
        }

        void on_udp_discovery_send_done(uv_udp_send_t *req, int status)
        {
            uv_udp_send_pool_t *send_pool = (uv_udp_send_pool_t *)req->data;
            uv_udp_send_release(send_pool, req);
        }

        void on_udp_discovery_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
        {
            // Handle UDP discovery packet reception
            // Validate the discovery packet and send a response back
            // The response should contain the server's IP address, TCP port, and UDP port
            server_discovery_t *server = (server_discovery_t *)handle->data;
            if (nread > 0 && addr)
            {
                servers_t *con_servers = server->m_owner;

                // Validate the discovery packet (implementation-specific)

                // Prepare response packet
                // For simplicity, we assume the response is already prepared in server->m_response_data
                uv_udp_send_t *send_req = uv_udp_send_acquire(con_servers->m_udp_send_pool);
                send_req->handle        = handle;
                send_req->data          = con_servers->m_udp_send_pool;  // For release callback
                uv_udp_send(send_req, &con_servers->m_udp_send_handle, &server->m_response_buffer, 1, addr, on_udp_discovery_send_done);
            }

            if (buf->base)
            {
                uv_buf_release(server->m_owner->m_buf_allocator, (byte *)buf->base);
            }
        }

        void on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
        {
            server_udp_t *server = (server_udp_t *)handle->data;
            if (nread > 0 && addr)
            {
                nmmmq::publish(server->m_stream, buf->base, (u32)nread);
            }

            if (buf->base)
            {
                uv_buf_release(server->m_owner->m_buf_allocator, (byte *)buf->base);
            }
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
malloc_based_allocator g_mba;

// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------

int main()
{
    alloc_t *allocator = &g_mba;

    ncore::config_main_t *config = ncore::g_load_config(allocator);
    if (!config)
    {
        printf("Failed to load configuration.\n");
        return -1;
    }

    // Create the event loop
    uv_loop_t *loop = uv_default_loop();

    // Create 'servers' context
    ncore::nconartist::servers_t *servers = ncore::nconartist::create_servers(allocator, loop, config, INITIAL_CONN_CAPACITY);
    ncore::nconartist::start_servers(allocator, servers);
    {
        // TODO: Have a function poll a (shared) memory mapped file for messages to send to TCP and UDP clients
        // TODO: Use ctui to create a simple terminal UI for monitoring connections showing basic info and stats
        uv_run(loop, UV_RUN_DEFAULT);
    }
    // Cleanup
    ncore::nconartist::destroy_servers(allocator, servers);

    uv_loop_close(loop);
    return 0;
}
