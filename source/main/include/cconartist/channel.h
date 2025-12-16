#ifndef CHANNEL_H
#define CHANNEL_H

#include "clibuv/uv.h"

namespace ncore
{
    class alloc_t;

    struct channel_t;

    channel_t *channel_init(alloc_t *allocator, size_t capacity);
    void       channel_destroy(channel_t *&ch);
    int        channel_push(channel_t *ch, void *data);
    void      *channel_pop(channel_t *ch);
    void      *channel_pop_nowait(channel_t *ch);

}  // namespace ncore

#endif
