#ifndef __CCONARTIST_UDP_SEND_POOL_H__
#define __CCONARTIST_UDP_SEND_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct user_id_register_t;

    // Initialize user-id array with given number of bits that are used for sharding.
    // Mostly you would aim to have around 4 to 8 items per shard if possible, so for
    // 1024 user-ids, using 8 bits for sharding would give 256 shards with on average
    // 4 user-ids per shard.
    user_id_register_t *user_id_register_create(alloc_t *allocator, i32 max_user_ids, u8 n_sharding_bits = 8);
    void                user_id_register_destroy(user_id_register_t *&r);
    i32                 user_id_register_add(user_id_register_t *r, u64 user_id);
    i32                 user_id_register_find(user_id_register_t *r, u64 user_id);

}  // namespace ncore
#endif
