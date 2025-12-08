#include "cconartist/packet_pool.h"

#include <string.h>

namespace ncore
{
    void packet_pool_init(size_t pool_size, packet_pool_t &pool)
    {
        if (pool.m_capacity != 0)
            return;  // Already initialized

        if (pool_size > INT16_MAX)
            pool_size = INT16_MAX;

        pool.m_packets   = (packet_t *)calloc(pool_size, sizeof(packet_t));
        pool.m_free_list = (int16_t *)malloc(pool_size * sizeof(int16_t));
        pool.m_capacity  = pool_size;
        pool.m_top       = (int16_t)pool_size - 1;
        for (int16_t i = 0; i < (int16_t)pool_size; i++)
            pool.m_free_list[i] = i;
        uv_mutex_init(&pool.m_mutex);
    }

    void packet_pool_release(packet_pool_t &pool)
    {
        uv_mutex_destroy(&pool.m_mutex);
        free(pool.m_packets);
        free(pool.m_free_list);

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
        int16_t idx = pool.m_free_list[pool.m_top--];
        uv_mutex_unlock(&pool.m_mutex);
        packet_t *pkt = &pool.m_packets[idx];
        pkt->m_conn   = NULL;
        pkt->m_size   = 0;
        return pkt;
    }

    void packet_release(packet_pool_t &pool, packet_t *pkt)
    {
        uv_mutex_lock(&pool.m_mutex);
        int16_t idx                    = (int16_t)(pkt - pool.m_packets);
        pool.m_free_list[++pool.m_top] = idx;
        uv_mutex_unlock(&pool.m_mutex);
    }
}  // namespace ncore
