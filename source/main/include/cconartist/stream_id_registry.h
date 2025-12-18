#ifndef __CCONARTIST_UDP_SEND_POOL_H__
#define __CCONARTIST_UDP_SEND_POOL_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "clibuv/uv.h"
#include "cconartist/types.h"

namespace ncore
{
    class alloc_t;

    // A registry where you can quickly get the stream-id that is associated with a given user-id.
    // Initialize stream-id registry with given number of bits that are used for sharding.
    // Mostly you would aim to have around 4 to 16 items per shard if possible, so for
    // 1024 items, using 7 bits for sharding would give 128 shards with an average of
    // 8 items per shard.
    stream_id_registry_t *stream_id_registry_create(alloc_t *allocator, i32 capacity, u8 n_sharding_bits = 8);
    void                  stream_id_registry_destroy(stream_id_registry_t *&r);
    void                  stream_id_register(stream_id_registry_t *r, u64 user_id, stream_id_t stream_id);
    bool                  stream_id_find(stream_id_registry_t *r, u64 user_id, stream_id_t &out_stream_id);

}  // namespace ncore
#endif
