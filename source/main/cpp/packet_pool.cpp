#include "cconartist/packet_pool.h"
#include "ccore/c_allocator.h"

#include <string.h>

namespace ncore
{
    void packet_pool_init(alloc_t *allocator, u16 pool_size, packet_pool_t &pool)
    {
        if (pool.m_allocator != nullptr)
            return;  // Already initialized

        if (pool_size > D_S16_MAX)
            pool_size = D_S16_MAX;

        pool.m_allocator = allocator;
        pool.m_packets   = g_allocate_array<packet_t>(allocator, pool_size);
        pool.m_free_list = g_allocate_array<u16>(allocator, pool_size);
        pool.m_capacity  = pool_size;
        pool.m_top       = (i32)pool_size - 1;
        for (i32 i = 0; i < (i32)pool_size; i++)
            pool.m_free_list[i] = i;
        uv_mutex_init(&pool.m_mutex);
    }

    void packet_pool_release(packet_pool_t &pool)
    {
        if (pool.m_allocator == nullptr)
            return;  // Not initialized

        uv_mutex_destroy(&pool.m_mutex);

        g_deallocate_array(pool.m_allocator, pool.m_packets);
        g_deallocate_array(pool.m_allocator, pool.m_free_list);

        pool.m_allocator = nullptr;
        pool.m_packets   = NULL;
        pool.m_free_list = NULL;
        pool.m_capacity  = 0;
        pool.m_top       = -1;
        pool.m_mutex     = {};
    }

    packet_t *packet_acquire(packet_pool_t &pool)
    {
        uv_mutex_lock(&pool.m_mutex);
        if (pool.m_top < 0)
        {
            uv_mutex_unlock(&pool.m_mutex);
            return NULL;
        }
        u16 idx = pool.m_free_list[pool.m_top];
        pool.m_top--;
        uv_mutex_unlock(&pool.m_mutex);
        packet_t *pkt = &pool.m_packets[idx];
        pkt->m_conn   = NULL;
        pkt->m_size   = 0;
        return pkt;
    }

    void packet_release(packet_pool_t &pool, packet_t *pkt)
    {
        uv_mutex_lock(&pool.m_mutex);
        u16 idx                    = (u16)(pkt - pool.m_packets);
        pool.m_free_list[++pool.m_top] = idx;
        uv_mutex_unlock(&pool.m_mutex);
    }
}  // namespace ncore
