#ifndef __CCONARTIST_UDP_SEND_POOL_H__
#define __CCONARTIST_UDP_SEND_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
#define SEND_POOL_SIZE 1024

    typedef struct udp_send_pool_t
    {
        uv_udp_send_t pool[SEND_POOL_SIZE];
        int16_t       free_list[SEND_POOL_SIZE];  // Holds indices of free items
        int16_t       free_count;                 // Number of free items
    } udp_send_pool_t;

    // Initialize pool
    void send_pool_init(udp_send_pool_t *send_pool)
    {
        send_pool->free_count = SEND_POOL_SIZE;
        for (int i = 0; i < SEND_POOL_SIZE; i++)
        {
            send_pool->free_list[i] = i;  // All items start free
        }
    }

    // Acquire a send request from the pool
    uv_udp_send_t *send_pool_acquire(udp_send_pool_t *send_pool)
    {
        if (send_pool->free_count == 0)
        {
            return NULL;  // Pool exhausted
        }
        int16_t index = send_pool->free_list[--send_pool->free_count];
        return &send_pool->pool[index];
    }

    // Release a send request back to the pool
    void send_pool_release(udp_send_pool_t *send_pool, uv_udp_send_t *req)
    {
        int16_t index = (int16_t)(req - send_pool->pool);
        if (index >= 0 && index < SEND_POOL_SIZE)
        {
            send_pool->free_list[send_pool->free_count++] = index;
        }
    }
}  // namespace ncore
#endif
