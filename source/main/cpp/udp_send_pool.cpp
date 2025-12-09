#include "cconartist/udp_send_pool.h"

#include "ccore/c_allocator.h"

#include <string.h>

namespace ncore
{
    struct udp_send_pool_t
    {
        uv_udp_send_t *m_pool;        // Pool of send requests
        u16           *m_free_list;   // Holds indices of free items
        u16            m_free_count;  // Number of free items
        u16            m_capacity;    // Total capacity of the pool
        alloc_t       *m_allocator;

        DCORE_CLASS_PLACEMENT_NEW_DELETE;
    };

    udp_send_pool_t *send_pool_create(alloc_t *allocator, u16 size)
    {
        udp_send_pool_t *send_pool = g_construct<udp_send_pool_t>(allocator);
        send_pool->m_free_count    = size;
        send_pool->m_capacity      = size;
        send_pool->m_pool          = g_allocate_array<uv_udp_send_t>(allocator, size);
        send_pool->m_free_list     = g_allocate_array<u16>(allocator, size);
        send_pool->m_allocator     = allocator;
        for (u16 i = 0; i < size; ++i)
            send_pool->m_free_list[i] = i;
        return send_pool;
    }

    void send_pool_destroy(udp_send_pool_t *&send_pool)
    {
        if (send_pool->m_pool != nullptr)
        {
            g_deallocate_array(send_pool->m_allocator, send_pool->m_pool);
            send_pool->m_pool = nullptr;
        }
        if (send_pool->m_free_list != nullptr)
        {
            g_deallocate_array(send_pool->m_allocator, send_pool->m_free_list);
            send_pool->m_free_list = nullptr;
        }
        g_deallocate_array(send_pool->m_allocator, send_pool);
        send_pool = nullptr;
    }

    // Acquire a send request from the pool
    uv_udp_send_t *send_pool_acquire(udp_send_pool_t *send_pool)
    {
        if (send_pool->m_free_count == 0)
        {
            return nullptr;  // Pool exhausted
        }
        u16            index = send_pool->m_free_list[--send_pool->m_free_count];
        uv_udp_send_t *item  = &send_pool->m_pool[index];
        *item                = uv_udp_send_t();  // Clear the structure
        return item;
    }

    // Release a send request back to the pool
    void send_pool_release(udp_send_pool_t *send_pool, uv_udp_send_t *item)
    {
        u16 index = (u16)(item - send_pool->m_pool);
        if (index >= 0 && index < send_pool->m_capacity)
        {
            send_pool->m_free_list[send_pool->m_free_count++] = index;
        }
    }

}  // namespace ncore
