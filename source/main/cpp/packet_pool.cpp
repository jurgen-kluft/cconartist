#include "cconartist/packet_pool.h"

#include "ccore/c_allocator.h"

#include "ccore/c_bin.h"

#include <string.h>

namespace ncore
{
    // The packet pool at the back-end is using a bin
    struct packet_pool_t
    {
        nbin::bin_t *m_packet_bin;
        nbin::bin_t *m_packet_data_bin;
        u32          m_packet_data_size;
        alloc_t     *m_allocator;
    };

    packet_pool_t *packet_pool_create(alloc_t *allocator, u32 initial_pool_size, u32 max_pool_size, u16 packet_data_size)
    {
        packet_pool_t *pool      = g_allocate_and_clear<packet_pool_t>(allocator);
        pool->m_allocator        = allocator;
        pool->m_packet_bin       = nbin::make_bin(sizeof(packet_t), max_pool_size);
        pool->m_packet_data_size = packet_data_size;
        pool->m_packet_data_bin  = nbin::make_bin(packet_data_size, max_pool_size);
        return pool;
    }

    void packet_pool_destroy(packet_pool_t *&pool)
    {
        if (pool == nullptr || pool->m_allocator == nullptr)
            return;
        nbin::destroy(pool->m_packet_bin);
        nbin::destroy(pool->m_packet_data_bin);
        alloc_t *allocator = pool->m_allocator;
        g_deallocate(allocator, pool);
    }

    packet_t *packet_acquire(packet_pool_t *pool)
    {
        packet_t *pkt        = (packet_t *)nbin::alloc(pool->m_packet_bin);
        pkt->m_data          = (u8 *)nbin::alloc(pool->m_packet_data_bin);
        pkt->m_data_size     = 0;
        pkt->m_data_capacity = pool->m_packet_data_size;
        return pkt;
    }

    void packet_release(packet_pool_t *pool, packet_t *pkt)
    {
        if (pkt == nullptr || pool == nullptr)
            return;
        nbin::free(pool->m_packet_data_bin, pkt->m_data);
        nbin::free(pool->m_packet_bin, pkt);
    }

}  // namespace ncore
