#ifndef __CCONARTIST_TCP_WRITE_POOL_H__
#define __CCONARTIST_TCP_WRITE_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
#define WRITE_POOL_SIZE 1024

    typedef struct tcp_write_pool_t
    {
        uv_write_t pool[WRITE_POOL_SIZE];
        int16_t    free_list[WRITE_POOL_SIZE];
        int16_t    free_count;
    } tcp_write_pool_t;

    // Initialize pool
    void write_pool_init(tcp_write_pool_t* write_pool)
    {
        write_pool->free_count = WRITE_POOL_SIZE;
        for (int i = 0; i < WRITE_POOL_SIZE; i++)
        {
            write_pool->free_list[i] = i;
        }
    }

    // Acquire a write request
    uv_write_t* write_pool_acquire(tcp_write_pool_t* write_pool)
    {
        if (write_pool->free_count == 0)
            return NULL;
        int16_t index = write_pool->free_list[--write_pool->free_count];
        return &write_pool->pool[index];
    }

    // Release a write request
    void write_pool_release(tcp_write_pool_t* write_pool, uv_write_t* req)
    {
        int16_t index = (int16_t)(req - write_pool->pool);
        if (index >= 0 && index < WRITE_POOL_SIZE)
        {
            write_pool->free_list[write_pool->free_count++] = index;
        }
    }
}  // namespace ncore
#endif
