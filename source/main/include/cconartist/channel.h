#ifndef CHANNEL_H
#define CHANNEL_H

#include "clibuv/uv.h"

namespace ncore
{
    struct node_t
    {
        void   *m_data;
        node_t *m_next;
    };

    struct channel_t
    {
        uv_mutex_t m_mutex;
        uv_cond_t  m_cond;
        node_t    *m_head;
        node_t    *m_tail;
        node_t    *m_free_list;
        node_t    *m_nodes;
        size_t     m_size;
        size_t     m_capacity;
    };

    int   channel_init(channel_t *ch, size_t capacity);
    void  channel_destroy(channel_t *ch);
    int   channel_push(channel_t *ch, void *data);
    void *channel_pop(channel_t *ch);

}  // namespace ncore

#endif
