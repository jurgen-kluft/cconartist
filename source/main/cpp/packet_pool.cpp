#include "cconartist/packet_pool.h"

#include "ccore/c_allocator.h"

#include "callocator/c_allocator_heap.h"

#include <string.h>

namespace ncore
{
    // The packet pool at the back-end is using a heap allocator which is based on TLSF and uses a
    // virtual memory arena to extend the heap as needed.

    struct packet_pool_t
    {
        alloc_t *m_packet_heap;
        alloc_t *m_allocator;
    };

    packet_pool_t *packet_pool_create(alloc_t *allocator, u32 initial_pool_size, u32 max_pool_size)
    {
        packet_pool_t *pool = g_allocate_and_clear<packet_pool_t>(allocator);
        pool->m_allocator   = allocator;
        pool->m_packet_heap = g_create_heap(initial_pool_size, max_pool_size);
        return pool;
    }

    void packet_pool_destroy(packet_pool_t *&pool)
    {
        if (pool == nullptr || pool->m_allocator == nullptr)
            return;

        g_release_heap(pool->m_packet_heap);
        g_deallocate(pool->m_allocator, pool);
    }

    packet_t *packet_acquire(packet_pool_t *pool, u32 packet_size)
    {
        packet_t *pkt        = g_allocate<packet_t>(pool->m_packet_heap);
        pkt->m_data          = g_allocate_array<u8>(pool->m_packet_heap, packet_size);
        pkt->m_data_size     = 0;
        pkt->m_data_capacity = packet_size;
        return pkt;
    }

    void packet_release(packet_pool_t *pool, packet_t *pkt)
    {
        g_deallocate(pool->m_packet_heap, pkt->m_data);
        g_deallocate(pool->m_packet_heap, pkt);
    }
}  // namespace ncore
