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
    typedef void (*job_fn_t)(void* job_data0, void* job_data1);

    // Job Manager
    // n_channels; each channel has its own completion ring, this allows different parts of the application to push jobs
    //            and wait for their completion independently.
    // n_threads: number of worker threads
    // pending_capacity: number of jobs that can be pending at once
    // completed_capacity: number of completed jobs that can be buffered per channel
    struct job_manager_t;
    typedef i32    job_channel_t;
    job_manager_t* create_job_manager(alloc_t* allocator, i32 max_channels, i32 n_threads, i32 pending_capacity);
    void           destroy_job_manager(job_manager_t*& manager);
    job_channel_t  init_channel(job_manager_t* jm, i32 completed_capacity);
    i32            push_job(job_manager_t* jm, job_channel_t channel, job_fn_t job_fn, void* job_data0, void* job_data1 = nullptr);
    i32            pop_job(job_manager_t* jm, job_channel_t channel, void*& job_data0, void*& job_data1);
    i32            pop_job_wait(job_manager_t* jm, job_channel_t channel, void*& job_data, void*& job_data1);

}  // namespace ncore

#endif
