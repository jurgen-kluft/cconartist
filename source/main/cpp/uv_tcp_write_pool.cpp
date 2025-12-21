#include "cconartist/uv_tcp_write_pool.h"
#include "ccore/c_allocator.h"

namespace ncore
{
    struct uv_tcp_write_pool_t
    {
        uv_write_t* m_pool;
        u16*        m_free_list;
        i32         m_free_count;
        i32         m_capacity;
        alloc_t*    m_allocator;

        DCORE_CLASS_PLACEMENT_NEW_DELETE;
    };

    uv_tcp_write_pool_t* uv_tcp_write_pool_create(alloc_t* allocator, u16 capacity)
    {
        uv_tcp_write_pool_t* write_pool = g_construct<uv_tcp_write_pool_t>(allocator);

        write_pool->m_pool       = g_allocate_array<uv_write_t>(allocator, capacity);
        write_pool->m_free_list  = g_allocate_array<u16>(allocator, capacity);
        write_pool->m_capacity   = capacity;
        write_pool->m_free_count = capacity;
        write_pool->m_allocator  = allocator;

        for (i32 i = 0; i < capacity; i++)
        {
            write_pool->m_free_list[i] = (u16)i;
        }

        return write_pool;
    }

    void uv_tcp_write_pool_destroy(uv_tcp_write_pool_t*& write_pool)
    {
        if (write_pool->m_pool != nullptr)
        {
            g_deallocate_array(write_pool->m_allocator, write_pool->m_pool);
            write_pool->m_pool = nullptr;
        }
        if (write_pool->m_free_list != nullptr)
        {
            g_deallocate_array(write_pool->m_allocator, write_pool->m_free_list);
            write_pool->m_free_list = nullptr;
        }
        g_deallocate_array(write_pool->m_allocator, write_pool);
        write_pool = nullptr;
    }

    // Acquire a write request
    uv_write_t* uv_tcp_write_acquire(uv_tcp_write_pool_t* write_pool)
    {
        if (write_pool->m_free_count == 0)
            return NULL;
        u16 index = write_pool->m_free_list[--write_pool->m_free_count];
        return &write_pool->m_pool[index];
    }

    // Release a write request
    void uv_tcp_write_release(uv_tcp_write_pool_t* write_pool, uv_write_t* req)
    {
        u16 index = (u16)(req - write_pool->m_pool);
        if (index >= 0 && index < write_pool->m_capacity)
        {
            write_pool->m_free_list[write_pool->m_free_count++] = (u16)index;
        }
    }

}  // namespace ncore
