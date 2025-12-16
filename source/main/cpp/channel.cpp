#include "cconartist/channel.h"
#include "ccore/c_allocator.h"

namespace ncore
{
    struct node_t
    {
        void   *m_data;
        node_t *m_next;
    };

    struct channel_t
    {
        alloc_t   *m_allocator;
        uv_mutex_t m_mutex;
        uv_cond_t  m_cond;
        node_t    *m_head;
        node_t    *m_tail;
        node_t    *m_free_list;
        node_t    *m_nodes;
        size_t     m_size;
        size_t     m_capacity;
    };

    channel_t *channel_init(alloc_t *allocator, size_t capacity)
    {
        channel_t *ch = g_allocate<channel_t>(allocator);
        if (uv_mutex_init(&ch->m_mutex) != 0)
        {
            allocator->deallocate(ch);
            return nullptr;
        }
        if (uv_cond_init(&ch->m_cond) != 0)
        {
            uv_mutex_destroy(&ch->m_mutex);
            allocator->deallocate(ch);
            return nullptr;
        }

        ch->m_allocator = allocator;
        ch->m_nodes     = g_allocate_array<node_t>(allocator, capacity);
        if (!ch->m_nodes)
        {
            uv_cond_destroy(&ch->m_cond);
            uv_mutex_destroy(&ch->m_mutex);
            allocator->deallocate(ch);
            return nullptr;
        }
        ch->m_capacity = capacity;
        ch->m_size     = 0;
        ch->m_head = ch->m_tail = NULL;
        ch->m_free_list         = &ch->m_nodes[0];
        for (size_t i = 0; i < capacity - 1; i++)
        {
            ch->m_nodes[i].m_next = &ch->m_nodes[i + 1];
        }
        ch->m_nodes[capacity - 1].m_next = NULL;
        return ch;
    }

    void channel_destroy(channel_t *& ch)
    {
        uv_mutex_destroy(&ch->m_mutex);
        uv_cond_destroy(&ch->m_cond);
        ch->m_allocator->deallocate(ch->m_nodes);
        ch->m_allocator->deallocate(ch);
    }

    int channel_push(channel_t *ch, void *data)
    {
        uv_mutex_lock(&ch->m_mutex);
        while (ch->m_size >= ch->m_capacity)
        {
            uv_cond_wait(&ch->m_cond, &ch->m_mutex);
        }
        node_t *n       = ch->m_free_list;
        ch->m_free_list = n->m_next;
        n->m_data       = data;
        n->m_next       = NULL;
        if (ch->m_tail)
            ch->m_tail->m_next = n;
        else
            ch->m_head = n;
        ch->m_tail = n;
        ch->m_size++;
        uv_cond_signal(&ch->m_cond);
        uv_mutex_unlock(&ch->m_mutex);
        return 0;
    }

    void *channel_pop(channel_t *ch)
    {
        uv_mutex_lock(&ch->m_mutex);
        while (ch->m_size == 0)
        {
            uv_cond_wait(&ch->m_cond, &ch->m_mutex);
        }
        node_t *n  = ch->m_head;
        ch->m_head = n->m_next;
        if (!ch->m_head)
            ch->m_tail = NULL;
        ch->m_size--;
        void *data      = n->m_data;
        n->m_next       = ch->m_free_list;
        ch->m_free_list = n;
        uv_cond_signal(&ch->m_cond);
        uv_mutex_unlock(&ch->m_mutex);
        return data;
    }

    void *channel_pop_nowait(channel_t *ch)
    {
        uv_mutex_lock(&ch->m_mutex);
        if (ch->m_size == 0)
        {
            uv_mutex_unlock(&ch->m_mutex);
            return nullptr;
        }
        node_t *n  = ch->m_head;
        ch->m_head = n->m_next;
        if (!ch->m_head)
            ch->m_tail = NULL;
        ch->m_size--;
        void *data      = n->m_data;
        n->m_next       = ch->m_free_list;
        ch->m_free_list = n;
        uv_cond_signal(&ch->m_cond);
        uv_mutex_unlock(&ch->m_mutex);
        return data;
    }
}  // namespace ncore
