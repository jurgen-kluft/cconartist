#include "cconartist/packet_pool.h"
#include "ccore/c_allocator.h"

#include <string.h>

namespace ncore
{
    struct packet_pool_t
    {
        packet_pool_t()
            : m_packet_array(nullptr)
            , m_packet_data(nullptr)
            , m_packet_max_data_size(0)
            , m_free_list(nullptr)
            , m_top(-1)
            , m_capacity(0)
            , m_allocator(nullptr)
        {
            m_mutex = {};
        }

        DCORE_CLASS_PLACEMENT_NEW_DELETE

        packet_t  *m_packet_array;
        u8        *m_packet_data;
        u32        m_packet_max_data_size;
        u16       *m_free_list;
        i32        m_top;
        i32        m_capacity;
        uv_mutex_t m_mutex;
        alloc_t   *m_allocator;
    };

    packet_pool_t *packet_pool_create(alloc_t *allocator, u16 pool_size, u16 max_packet_data_size)
    {
        packet_pool_t *pool = g_construct<packet_pool_t>(allocator);

        if (pool_size > D_S16_MAX)
            pool_size = D_S16_MAX;

        pool->m_allocator            = allocator;
        pool->m_packet_array         = g_allocate_array<packet_t>(allocator, pool_size);
        pool->m_packet_data          = g_allocate_array<u8>(allocator, pool_size * max_packet_data_size);
        pool->m_packet_max_data_size = max_packet_data_size;
        pool->m_free_list            = g_allocate_array<u16>(allocator, pool_size);
        pool->m_capacity             = pool_size;
        pool->m_top                  = (i32)pool_size - 1;
        for (i32 i = 0; i < (i32)pool_size; i++)
            pool->m_free_list[i] = i;
        uv_mutex_init(&pool->m_mutex);

        return pool;
    }

    void packet_pool_destroy(packet_pool_t *&pool)
    {
        if (pool == nullptr)
            return;

        if (pool->m_allocator == nullptr)
            return;  // Not initialized

        uv_mutex_destroy(&pool->m_mutex);

        g_deallocate_array(pool->m_allocator, pool->m_packet_array);
        g_deallocate_array(pool->m_allocator, pool->m_packet_data);
        g_deallocate_array(pool->m_allocator, pool->m_free_list);

        g_deallocate_array(pool->m_allocator, pool);
        pool = nullptr;
    }

    void packet_pool_release(packet_pool_t *&pool)
    {
        if (pool->m_allocator == nullptr)
            return;  // Not initialized

        uv_mutex_destroy(&pool->m_mutex);

        g_deallocate_array(pool->m_allocator, pool->m_packet_array);
        g_deallocate_array(pool->m_allocator, pool->m_packet_data);
        g_deallocate_array(pool->m_allocator, pool->m_free_list);

        g_deallocate_array(pool->m_allocator, pool);
        pool = nullptr;
    }

    packet_t *packet_acquire(packet_pool_t *pool)
    {
        uv_mutex_lock(&pool->m_mutex);
        if (pool->m_top < 0)
        {
            uv_mutex_unlock(&pool->m_mutex);
            return NULL;
        }
        u16 idx = pool->m_free_list[pool->m_top];
        pool->m_top--;
        uv_mutex_unlock(&pool->m_mutex);
        packet_t *pkt        = &pool->m_packet_array[idx];
        pkt->m_data          = &pool->m_packet_data[(u32)idx * pool->m_packet_max_data_size];
        pkt->m_data_capacity = pool->m_packet_max_data_size;
        pkt->m_conn          = NULL;
        pkt->m_data_size     = 0;
        return pkt;
    }

    void packet_release(packet_pool_t *pool, packet_t *pkt)
    {
        uv_mutex_lock(&pool->m_mutex);
        u16 idx                          = (u16)(pkt - pool->m_packet_array);
        pool->m_free_list[++pool->m_top] = idx;
        uv_mutex_unlock(&pool->m_mutex);
    }
}  // namespace ncore
