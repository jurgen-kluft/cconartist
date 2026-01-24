#include "ccore/c_target.h"
#include "ccore/c_allocator.h"

#include "cmmio/c_mmio.h"
#include "cmmio/c_mmmq.h"

#include "cconartist/config.h"
#include "cconartist/conman.h"

#include "cconartist/unix_socket_server.h"

namespace ncore
{
    namespace nconartist
    {

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
struct client_t
{
};

struct server_t
{
    i32       m_num_clients;
    client_t *m_clients;
};

struct servers_t
{
    i32       m_num_servers;
    server_t *m_servers;
};

void on_msg_cb(i32 server_id, i32 client_fd, const void *data, uint_t len, void *user)
{
    // todo
}

void on_client_cb(i32 server_id, i32 client_fd, void *user)
{
    // todo
}

int main()
{
    alloc_t *allocator = &g_mba;

    ncore::config_main_t *config = ncore::g_load_config(allocator);
    if (!config)
    {
        printf("Failed to load configuration.\n");
        return -1;
    }

    us_loop *loop = us_loop_create(allocator);

    for (i32 i = 0; i < config->m_num_servers; ++i)
    {
        ncore::config_server_t &srv_cfg = config->m_servers[i];
        i32                     srv_id  = us_server_create(loop, srv_cfg.m_sock_path, (u32)strlen(srv_cfg.m_sock_path), 16 * cKB, on_msg_cb, on_client_cb, on_client_cb, nullptr);
        if (srv_id < 0)
        {
            printf("Failed to create server for socket path: %s\n", srv_cfg.m_sock_path);
            return -1;
        }
        printf("Server created with id %d for socket path: %s\n", srv_id, srv_cfg.m_sock_path);
    }

    return 0;
}
