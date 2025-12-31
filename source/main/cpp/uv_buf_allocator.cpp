#include "ccore/c_allocator.h"
#include "callocator/c_allocator_heap.h"

#include "clibuv/uv.h"

#include "cconartist/uv_buf_allocator.h"

namespace ncore
{
    struct uv_buf_alloc_t
    {
        alloc_t* m_allocator;
        alloc_t* m_buf_allocator;
    };

    uv_buf_alloc_t* uv_buf_alloc_create(alloc_t* allocator, u32 initial_capacity, u32 max_capacity)
    {
        uv_buf_alloc_t* alloc  = g_allocate<uv_buf_alloc_t>(allocator);
        alloc->m_allocator     = allocator;
        alloc->m_buf_allocator = g_create_heap(initial_capacity, max_capacity);
        return alloc;
    }

    void uv_buf_alloc_destroy(uv_buf_alloc_t*& alloc)
    {
        if (alloc)
        {
            g_release_heap(alloc->m_buf_allocator);
            alloc->m_allocator->deallocate(alloc);
            alloc = nullptr;
        }
    }

    byte* uv_buf_alloc(uv_buf_alloc_t* alloc, u32 buf_size)
    {
        void* buf = alloc->m_buf_allocator->allocate(buf_size);
        return (byte*)buf;
    }

    void uv_buf_release(uv_buf_alloc_t* alloc, byte* buf_data)
    {
        // deallocate the buffer
        alloc->m_buf_allocator->deallocate(buf_data);
    }

}  // namespace ncore
