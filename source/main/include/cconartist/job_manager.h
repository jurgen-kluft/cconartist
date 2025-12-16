#ifndef __CCONARTIST_JOB_MANAGER_H__
#define __CCONARTIST_JOB_MANAGER_H__
#include "ccore/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

namespace ncore
{
    class alloc_t;

    //------------------------------------------------------------------------------
    // Job interface
    //------------------------------------------------------------------------------
    typedef void (*job_fn_t)(void* job_data);

    struct job_manager_t;

    job_manager_t* create_job_manager(alloc_t* allocator, i32 n_threads, i32 pending_capacity, i32 completed_capacity);
    void           destroy_job_manager(job_manager_t*& manager);

    i32 submit(job_manager_t* jm, job_fn_t job_fn, void* job_data);
    i32 pop(job_manager_t* jm, void*& job_data);
    i32 pop_wait(job_manager_t* jm, void*& job_data);

}  // namespace ncore

#endif
