#include "cconartist/user_id_register.h"
#include "ccore/c_allocator.h"

namespace ncore
{
    // The main purpose of this structure is to improve the search of 'user_id' -> index.
    // 'user-id' is an 8 byte value, a certain amount of continuous-bits out of that value
    // is used to index into the shards array.
    struct user_id_register_t
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

        alloc_t *m_allocator;
        u64     *m_user_ids;           // The array of user ids
        i32      m_user_ids_capacity;  // The capacity of the user ids array
        i32      m_user_ids_size;      // The number of user ids added
        i32      m_n_shards;
        shard_t *m_shards;
    };

    user_id_register_t *user_id_register_create(alloc_t *allocator, i32 max_user_ids, u8 n_sharding_bits)
    {
        const i32           n_shards = 1 << n_sharding_bits;
        user_id_register_t *a        = (user_id_register_t *)allocator->allocate(sizeof(user_id_register_t));
        a->m_allocator               = allocator;
        a->m_user_ids                = (u64 *)allocator->allocate(sizeof(u64) * max_user_ids);
        a->m_user_ids_capacity       = max_user_ids;
        a->m_user_ids_size           = 0;
        a->m_n_shards                = n_shards;
        a->m_shards                  = (user_id_register_t::shard_t *)allocator->allocate(sizeof(user_id_register_t::shard_t) * n_shards);
        return a;
    }

    void user_id_register_destroy(user_id_register_t *&a)
    {
        if (a != nullptr)
        {
            for (i32 i = 0; i < a->m_n_shards; i++)
            {
                user_id_register_t::shard_t &shard = a->m_shards[i];
                if (shard.m_sorted != nullptr)
                    a->m_allocator->deallocate(shard.m_sorted);
            }
            a->m_allocator->deallocate(a->m_user_ids);
            a->m_allocator->deallocate(a->m_shards);
            a->m_allocator->deallocate(a);
            a = nullptr;
        }
    }

    i32 user_id_register_add(user_id_register_t *a, u64 user_id)
    {
        const i32 found_index = user_id_register_find(a, user_id);
        if (found_index >= 0)
            return found_index;
        const u32                    shard_index = (u32)((user_id >> 32) & (a->m_n_shards - 1));
        user_id_register_t::shard_t &shard       = a->m_shards[shard_index];
        const i32                    index       = a->m_user_ids_size;
        a->m_user_ids[a->m_user_ids_size++]      = user_id;
        shard.add(a->m_allocator, a->m_user_ids, index);
        return a->m_user_ids_size - 1;
    }

    i32 user_id_register_find(user_id_register_t *a, u64 user_id)
    {
        const u32                    shard_index = (u32)((user_id >> 32) & (a->m_n_shards - 1));
        user_id_register_t::shard_t &shard       = a->m_shards[shard_index];
        return shard.find(a->m_user_ids, user_id);
    }

}  // namespace ncore
