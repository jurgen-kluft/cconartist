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
        // todo
        return nullptr;
    }

    void uv_buf_alloc_destroy(uv_buf_alloc_t*& alloc)
    {
        // todo
    }

    byte* uv_buf_alloc(uv_buf_alloc_t* alloc, u32 buf_size)
    {
        // todo
        return nullptr;
    }

    void uv_buf_release(uv_buf_alloc_t* alloc, byte* buf_data)
    {
        // todo
    }

}  // namespace ncore
