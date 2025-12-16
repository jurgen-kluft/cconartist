#ifndef __CCONARTIST_TYPES_H__
#define __CCONARTIST_TYPES_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    namespace estream_type
    {
        typedef s8 enum_t;
        enum
        {
            TypeInvalid  = -1,
            TypeU8       = 0x01,
            TypeU16      = 0x12,
            TypeU32      = 0x24,
            TypeF32      = 0x34,
            TypeFixed    = 0x40,
            TypeVariable = 0x50,
        };
    }  // namespace estream_type

    const i32 c_relative_time_byte_count = 5;  // Number of bytes used to store relative time in stream

    namespace nmmio
    {
        struct mappedfile_t;
    }

    typedef i32 stream_id_t;

    struct stream_manager_t;
    struct stream_iterator_t;

    struct stream_request_manager_t;

    struct job_manager_t;

}  // namespace ncore

#endif
