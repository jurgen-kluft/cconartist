#ifndef __CCONARTIST_STREAM_REQUEST_H__
#define __CCONARTIST_STREAM_REQUEST_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

#include "cconartist/types.h"

namespace ncore
{
    class alloc_t;

    stream_request_manager_t* create_stream_request_manager(alloc_t* allocator, job_manager_t* jm, f64 now, const char* streams_basepath, const char* mappings_filepath);
    void                      destroy_stream_request_manager(stream_request_manager_t*& manager);
    void                      update_stream_requests(stream_request_manager_t* srm, f64 now);
    void                      push_stream_request(stream_request_manager_t* srm, u64 user_id, stream_id_t stream_id, u64 mmfile_size, nmmio::mappedfile_t* mmfile);
    bool                      pop_stream_request(stream_request_manager_t* srm, u64& user_id, nmmio::mappedfile_t*& out_mmfile);

}  // namespace ncore

#endif
