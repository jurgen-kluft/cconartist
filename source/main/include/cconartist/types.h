#ifndef __CCONARTIST_TYPES_H__
#define __CCONARTIST_TYPES_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    const i32 c_relative_time_byte_count = 5;  // Number of bytes used to store relative time in stream

    namespace nmmio
    {
        struct mappedfile_t;
    }

    typedef u32       stream_id_t;
    const stream_id_t c_invalid_stream_id = 0xFFFFFFFF;

    struct stream_manager_t;
    struct stream_iterator_t;
    struct stream_id_registry_t;
    struct stream_request_manager_t;

    struct job_manager_t;

}  // namespace ncore

#endif
