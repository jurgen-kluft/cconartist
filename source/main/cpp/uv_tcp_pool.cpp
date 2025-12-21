#include "cconartist/uv_tcp_pool.h"
#include "ccore/c_allocator.h"

#include "clibuv/uv.h"

namespace ncore
{
    struct uv_tcp_pool_t
    {
        uv_tcp_t* m_pool;
        u16*      m_free_list;
        i32       m_free_count;
        i32       m_capacity;
        alloc_t*    m_allocator;

        DCORE_CLASS_PLACEMENT_NEW_DELETE;
    };

    uv_tcp_pool_t* uv_tcp_pool_create(alloc_t* allocator, u32 capacity)
    {
        uv_tcp_pool_t* pool = g_construct<uv_tcp_pool_t>(allocator);

        pool->m_pool       = g_allocate_array<uv_tcp_t>(allocator, capacity);
        pool->m_free_list  = g_allocate_array<u16>(allocator, capacity);
        pool->m_capacity   = capacity;
        pool->m_free_count = capacity;
        pool->m_allocator  = allocator;

        for (u32 i = 0; i < capacity; i++)
        {
            pool->m_free_list[i] = (u16)i;
        }

        return pool;
    }

    void uv_tcp_pool_destroy(uv_tcp_pool_t*& pool)
    {
        if (pool->m_pool != nullptr)
        {
            g_deallocate_array(pool->m_allocator, pool->m_pool);
            pool->m_pool = nullptr;
        }
        if (pool->m_free_list != nullptr)
        {
            g_deallocate_array(pool->m_allocator, pool->m_free_list);
            pool->m_free_list = nullptr;
        }
        g_deallocate_array(pool->m_allocator, pool);
        pool = nullptr;
    }

    // Acquire a TCP handle
    uv_tcp_t* uv_tcp_acquire(uv_tcp_pool_t* pool)
    {
        if (pool->m_free_count == 0)
            return NULL;
        u16 index = pool->m_free_list[--pool->m_free_count];
        return &pool->m_pool[index];
    }

    // Release a TCP handle
    void uv_tcp_release(uv_tcp_pool_t* pool, uv_tcp_t* uv)
    {
        u16 index = (u16)(uv - pool->m_pool);
        if (index >= 0 && index < pool->m_capacity)
        {
            pool->m_free_list[pool->m_free_count++] = (u16)index;
        }
    }

}  // namespace ncore
