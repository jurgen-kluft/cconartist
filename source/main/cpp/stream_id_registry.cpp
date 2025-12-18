#include "cconartist/stream_id_registry.h"
#include "ccore/c_allocator.h"

namespace ncore
{
    // The main purpose of this structure is to improve the search of 'user_id' -> index.
    // 'user-id' is an 8 byte value, a certain amount of continuous-bits out of that value
    // is used to index into the shards array.
    struct stream_id_registry_t
    {
        struct shard_t
        {
            inline shard_t()
                : m_count(0)
                , m_capacity(0)
                , m_sorted(nullptr)
            {
            }

            i16  m_count;
            i16  m_capacity;
            i16 *m_sorted;

            void add(alloc_t *allocator, u64 *user_ids, i16 index)
            {
                if (m_count >= m_capacity)
                {
                    i32  new_capacity = m_capacity == 0 ? 4 : m_capacity * 2;
                    i16 *new_sorted   = (i16 *)allocator->allocate(sizeof(i16) * new_capacity);
                    for (i32 i = 0; i < m_count; i++)
                        new_sorted[i] = m_sorted[i];
                    if (m_sorted != nullptr)
                        allocator->deallocate(m_sorted);
                    m_capacity = new_capacity;
                    m_sorted   = new_sorted;
                }
                // Insert user_id in sorted order
                i16 pos = 0;
                while (pos < m_count && user_ids[m_sorted[pos]] < user_ids[index])
                    pos++;
                for (i16 i = m_count; i > pos; i--)
                    m_sorted[i] = m_sorted[i - 1];
                m_sorted[pos] = (i16)m_count;
                m_count++;
            }

            i16 find(u64 *user_ids, u64 user_id) const
            {
                // TODO check if a binary search is really needed here, since we might
                // be dealing with (1 <= n <= 8) elements in mostly all cases.
                for (i16 i = 0; i < m_count; i++)
                {
                    const i16 index = m_sorted[i];
                    if (user_ids[index] == user_id)
                        return index;
                }
                return -1;  // Not found
            }
        };

        alloc_t     *m_allocator;
        u64         *m_user_ids;    // The array of user ids
        stream_id_t *m_stream_ids;  // The array of stream ids
        i32          m_capacity;    // The capacity of the user ids array
        i32          m_size;        // The number of user ids added
        i32          m_n_shards;
        shard_t     *m_shards;
    };

    stream_id_registry_t *stream_id_register_create(alloc_t *allocator, i32 capacity, u8 n_sharding_bits)
    {
        const i32             n_shards = 1 << n_sharding_bits;
        stream_id_registry_t *r        = (stream_id_registry_t *)allocator->allocate(sizeof(stream_id_registry_t));
        r->m_allocator                 = allocator;
        r->m_user_ids                  = (u64 *)allocator->allocate(sizeof(u64) * capacity);
        r->m_stream_ids                = (stream_id_t *)allocator->allocate(sizeof(stream_id_t) * capacity);
        r->m_capacity                  = capacity;
        r->m_size                      = 0;
        r->m_n_shards                  = n_shards;
        r->m_shards                    = (stream_id_registry_t::shard_t *)allocator->allocate(sizeof(stream_id_registry_t::shard_t) * n_shards);
        return r;
    }

    void stream_id_register_destroy(stream_id_registry_t *&r)
    {
        if (r != nullptr)
        {
            for (i32 i = 0; i < r->m_n_shards; i++)
            {
                stream_id_registry_t::shard_t &shard = r->m_shards[i];
                if (shard.m_sorted != nullptr)
                    r->m_allocator->deallocate(shard.m_sorted);
            }
            r->m_allocator->deallocate(r->m_user_ids);
            r->m_allocator->deallocate(r->m_shards);
            r->m_allocator->deallocate(r);
            r = nullptr;
        }
    }

    stream_id_t stream_id_register(stream_id_registry_t *r, u64 user_id)
    {
        stream_id_t out_stream_id;
        if (stream_id_find(r, user_id, out_stream_id))
            return out_stream_id;
        const u32                      shard_index = (u32)((user_id >> 32) & (r->m_n_shards - 1));
        stream_id_registry_t::shard_t &shard       = r->m_shards[shard_index];
        const i32                      index       = r->m_size;
        r->m_user_ids[r->m_size]                   = user_id;
        r->m_stream_ids[r->m_size]                 = index;
        shard.add(r->m_allocator, r->m_user_ids, index);
        r->m_size++;
        return index;
    }

    bool stream_id_find(stream_id_registry_t *r, u64 user_id, stream_id_t &out_stream_id)
    {
        const u32                      shard_index = (u32)((user_id >> 32) & (r->m_n_shards - 1));
        stream_id_registry_t::shard_t &shard       = r->m_shards[shard_index];
        const i16                      found_index = shard.find(r->m_user_ids, user_id);
        if (found_index >= 0)
        {
            out_stream_id = r->m_stream_ids[found_index];
            return true;
        }
        return false;
    }

}  // namespace ncore
