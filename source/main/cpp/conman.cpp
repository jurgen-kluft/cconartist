#include "cconartist/conman.h"
#include "ccore/c_allocator.h"
#include "ccore/c_memory.h"

namespace ncore
{
    struct tcp_con_mgr_t
    {
        alloc_t   *m_allocator;
        tcp_con_t *m_conn_array;
        i32       *m_conn_free;
        u32        m_num_free;
        u32        m_capacity;
    };

    tcp_con_mgr_t *tcp_con_mgr_create(alloc_t *allocator, u32 max_capacity)
    {
        tcp_con_mgr_t *mgr = g_allocate_and_clear<tcp_con_mgr_t>(allocator);
        mgr->m_allocator   = allocator;
        mgr->m_conn_array  = g_allocate_array_and_clear<tcp_con_t>(allocator, max_capacity);
        mgr->m_conn_free   = g_allocate_array<i32>(allocator, max_capacity);
        mgr->m_num_free    = max_capacity;
        mgr->m_capacity    = max_capacity;
        for (u32 i = 0; i < max_capacity; i++)
            mgr->m_conn_free[i] = (i32)(max_capacity - 1 - i);

        return 0;
    }

    void tcp_con_mgr_destroy(tcp_con_mgr_t *mgr)
    {
        alloc_t *allocator = mgr->m_allocator;

        allocator->deallocate(mgr->m_conn_free);
        allocator->deallocate(mgr->m_conn_array);

        allocator->deallocate(mgr);
    }

    tcp_con_t *tcp_con_alloc(tcp_con_mgr_t *mgr)
    {
        if (mgr->m_num_free > 0)
        {
            mgr->m_num_free--;
            const i32  conn_index = mgr->m_conn_free[mgr->m_num_free];
            tcp_con_t *info       = &mgr->m_conn_array[conn_index];
            return info;
        }
        return nullptr;
    }

    void tcp_con_free(tcp_con_mgr_t *mgr, tcp_con_t *info)
    {
        nmem::memclr(info, sizeof(tcp_con_t));
        const i32 conn_index              = (i32)(info - mgr->m_conn_array);
        mgr->m_conn_free[mgr->m_num_free] = conn_index;
        mgr->m_num_free++;
    }

}  // namespace ncore
