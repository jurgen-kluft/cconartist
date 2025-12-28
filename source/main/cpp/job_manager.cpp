#include "ccore/c_allocator.h"
#include "ccore/c_math.h"
#include "ccore/c_memory.h"
#include "ccore/c_arena.h"
#include "cbase/c_runes.h"

#include "cconartist/job_manager.h"

#include "cmmio/c_mmio.h"
#include "clibuv/uv.h"

#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>

namespace ncore
{
    struct job_t
    {
        i32      m_channel;    // channel index
        job_fn_t m_job_fn;     // function pointer to execute the job
        void*    m_job_data0;  // optional user data pointer 0
        void*    m_job_data1;  // optional user data pointer 1
    };

    //------------------------------------------------------------------------------
    // Fixed-size circular buffer for job_t*
    //------------------------------------------------------------------------------
    struct ring_t
    {
        alloc_t* m_allocator;
        job_t*   m_buf;
        i32      m_capacity;  // total slots
        i32      m_head;      // index of next pop
        i32      m_tail;      // index of next push
        i32      m_count;     // number of elements

        // Initialize with given capacity
        i32 init(alloc_t* allocator, i32 cap)
        {
            m_allocator = allocator;
            m_buf       = g_allocate_array<job_t>(allocator, cap);
            if (!m_buf)
                return -1;
            m_capacity = cap;
            m_head     = 0;
            m_tail     = 0;
            m_count    = 0;
            return 0;
        }

        void destroy()
        {
            if (m_buf)
            {
                m_allocator->deallocate(m_buf);
                m_buf = NULL;
            }
            m_capacity = 0;
            m_head = m_tail = m_count = 0;
        }

        // Non-blocking push: returns 0 on success, -1 if full
        i32 push(i32 channel, job_fn_t job_fn, void* job_data0, void* job_data1)
        {
            if (m_count == m_capacity)
                return -1;
            m_buf[m_tail].m_channel   = channel;
            m_buf[m_tail].m_job_fn    = job_fn;
            m_buf[m_tail].m_job_data0 = job_data0;
            m_buf[m_tail].m_job_data1 = job_data1;
            m_tail                    = (m_tail + 1) % m_capacity;
            m_count += 1;
            return 0;
        }

        // Non-blocking pop: returns 0 on success, -1 if empty
        i32 pop(i32& channel, job_fn_t& job_fn, void*& job_data0, void*& job_data1)
        {
            if (m_count == 0)
                return -1;
            channel   = m_buf[m_head].m_channel;
            job_fn    = m_buf[m_head].m_job_fn;
            job_data0 = m_buf[m_head].m_job_data0;
            m_head    = (m_head + 1) % m_capacity;
            m_count -= 1;
            return 0;
        }
    };

    //------------------------------------------------------------------------------
    // job_manager_t: manages worker m_threads and two rings (pending/completed)
    // Everything is public, as requested.
    //------------------------------------------------------------------------------
    struct job_manager_t
    {
        DCORE_CLASS_PLACEMENT_NEW_DELETE

        // Configuration / state
        alloc_t*     m_allocator;
        uv_thread_t* m_threads;
        i32          m_thread_count;
        i32          m_max_channels;
        i32          m_n_channels;

        // Synchronization
        uv_mutex_t m_mutex;
        uv_cond_t  m_has_jobs;        // signaled when pending has jobs
        uv_cond_t* m_has_completed;   // signaled when completed has jobs (per channel)
        uv_cond_t* m_room_completed;  // signaled when producer pops from completed (per channel)

        volatile i32 m_stopping;    // 0->running, 1->stopping (drain or drop)
        i32          m_drain_mode;  // 1->drain pending; 0->drop pending immediately

        // Queues
        ring_t  m_pending;
        ring_t* m_completed;  // per-channel completed rings

        // Constructor
        void intialize(alloc_t* allocator, i32 max_channels, i32 n_threads, i32 pending_capacity)
        {
            m_allocator    = allocator;
            m_threads      = NULL;
            m_thread_count = n_threads;
            m_max_channels = max_channels;
            m_n_channels   = 0;
            m_stopping     = 0;
            m_drain_mode   = 1;

            if (m_thread_count <= 0)
                m_thread_count = 1;
            if (pending_capacity <= 0)
                pending_capacity = 1;

            if (m_pending.init(allocator, pending_capacity) != 0)
            {
                fprintf(stderr, "Failed to allocate rings.\n");
            }

            m_completed      = g_allocate_array<ring_t>(allocator, max_channels);
            m_has_completed  = g_allocate_array<uv_cond_t>(allocator, max_channels);
            m_room_completed = g_allocate_array<uv_cond_t>(allocator, max_channels);

            uv_mutex_init(&m_mutex);
            uv_cond_init(&m_has_jobs);

            m_threads = g_allocate_array<uv_thread_t>(m_allocator, m_thread_count);
            if (!m_threads)
            {
                fprintf(stderr, "Failed to allocate thread array.\n");
                m_stopping = 1;
                return;
            }

            i32 i;
            for (i = 0; i < m_thread_count; ++i)
            {
                i32 rc = uv_thread_create(&m_threads[i], thread_entry, this);
                if (rc != 0)
                {
                    fprintf(stderr, "uv_thread_create failed for worker %d (rc=%d)\n", i, rc);
                }
            }
        }

        job_channel_t init_channel(job_manager_t* jm, i32 completed_capacity)
        {
            if (completed_capacity <= 0)
                completed_capacity = 1;
            if (jm->m_n_channels >= jm->m_max_channels)
                return (job_channel_t)-1;

            job_channel_t channel = jm->m_n_channels++;
            uv_cond_init(&m_has_completed[channel]);
            uv_cond_init(&m_room_completed[channel]);
            if (m_completed[channel].init(jm->m_allocator, completed_capacity) != 0)
            {
                fprintf(stderr, "Failed to allocate rings.\n");
            }
            return channel;
        }

        // Destructor
        void shutdown()
        {
            // Stop and drain by default
            stop(1);

            if (m_threads)
            {
                m_allocator->deallocate(m_threads);
                m_threads = NULL;
            }

            uv_cond_destroy(&m_has_jobs);
            uv_mutex_destroy(&m_mutex);

            m_pending.destroy();

            for (i32 i = 0; i < m_n_channels; ++i)
            {
                uv_cond_destroy(&m_has_completed[i]);
                uv_cond_destroy(&m_room_completed[i]);
                m_completed[i].destroy();
            }
            g_deallocate_array(m_allocator, m_has_completed);
            g_deallocate_array(m_allocator, m_room_completed);
            g_deallocate_array(m_allocator, m_completed);
        }

        // Submit a job into the pending ring (non-blocking).
        // Returns 0 on success, -1 if m_stopping or queue is full.
        i32 submit(job_channel_t channel, job_fn_t job_fn, void* job_data0, void* job_data1)
        {
            if (job_fn == NULL || job_data0 == NULL)
                return -1;

            uv_mutex_lock(&m_mutex);
            if (m_stopping != 0)
            {
                uv_mutex_unlock(&m_mutex);
                return -1;
            }
            i32 rc = m_pending.push(channel, job_fn, job_data0, job_data1);
            if (rc == 0)
            {
                uv_cond_signal(&m_has_jobs);  // wake one worker
            }
            uv_mutex_unlock(&m_mutex);
            return rc;
        }

        // Pop a completed job (non-blocking).
        // Returns 0 on success with *out set; -1 if none available.
        i32 pop_completed(job_channel_t channel, void*& job_data0, void*& job_data1)
        {
            uv_mutex_lock(&m_mutex);
            i32      channel_out;
            job_fn_t job_fn;
            i32      rc = m_completed[channel].pop(channel_out, job_fn, job_data0, job_data1);
            if (rc == 0)
            {
                // Make room for workers that may be waiting to push into completed
                uv_cond_signal(&m_room_completed[channel]);
            }
            uv_mutex_unlock(&m_mutex);
            return rc;
        }

        // Optional: Pop a completed job, blocking until one is available
        // or until the manager is stopped and no more completions will arrive.
        // Returns 0 if popped; -1 if interrupted or no more items expected.
        i32 pop_completed_wait(job_channel_t channel, void*& job_data0, void*& job_data1)
        {
            uv_mutex_lock(&m_mutex);
            while (m_completed[channel].m_count == 0)
            {
                if (m_stopping != 0 && m_pending.m_count == 0)
                {
                    // If m_stopping and no pending work remains, no more completions expected.
                    uv_mutex_unlock(&m_mutex);
                    return -1;
                }
                uv_cond_wait(&m_has_completed[channel], &m_mutex);
            }
            i32      channel_out;
            job_fn_t job_fn;
            i32      rc = m_completed[channel].pop(channel_out, job_fn, job_data0, job_data1);
            if (rc == 0)
            {
                uv_cond_signal(&m_room_completed[channel]);
            }
            uv_mutex_unlock(&m_mutex);
            return rc;
        }

        // Stop:
        // drain = 1 -> finish all queued jobs before exiting workers
        // drain = 0 -> drop queued jobs immediately
        void stop(i32 drain)
        {
            uv_mutex_lock(&m_mutex);
            if (m_stopping != 0)
            {
                uv_mutex_unlock(&m_mutex);
                // Threads may already be joining or joined
            }
            else
            {
                m_stopping   = 1;
                m_drain_mode = (drain ? 1 : 0);

                if (m_drain_mode == 0)
                {
                    // Drop all pending jobs immediately (ownership remains with caller)
                    m_pending.m_head  = 0;
                    m_pending.m_tail  = 0;
                    m_pending.m_count = 0;
                }

                // Wake all workers and also any producer waiting for completions
                uv_cond_broadcast(&m_has_jobs);
                for (i32 i = 0; i < m_n_channels; ++i)
                {
                    uv_cond_broadcast(&m_has_completed[i]);
                    uv_cond_broadcast(&m_room_completed[i]);
                }
                uv_mutex_unlock(&m_mutex);

                // Join all worker m_threads
                if (m_threads)
                {
                    i32 i;
                    for (i = 0; i < m_thread_count; ++i)
                    {
                        uv_thread_join(&m_threads[i]);
                    }
                }
            }
        }

        // Diagnostic: approximate number of pending jobs
        i32 pending_count()
        {
            i32 s;
            uv_mutex_lock(&m_mutex);
            s = m_pending.m_count;
            uv_mutex_unlock(&m_mutex);
            return s;
        }

        // --- Worker implementation ---

        static void thread_entry(void* arg)
        {
            job_manager_t* self = (job_manager_t*)arg;
            self->worker_loop();
        }

        void worker_loop()
        {
            uv_mutex_lock(&m_mutex);
            for (;;)
            {
                // Wait until there is a job or stop requested
                while (m_pending.m_count == 0 && m_stopping == 0)
                {
                    uv_cond_wait(&m_has_jobs, &m_mutex);
                }

                // If m_stopping and pending is empty (drain complete or drop), exit
                if (m_stopping != 0 && m_pending.m_count == 0)
                {
                    uv_mutex_unlock(&m_mutex);
                    break;
                }

                // Pop one job from pending
                i32      channel;
                void*    job_data0 = NULL;
                void*    job_data1 = NULL;
                job_fn_t job_fn   = NULL;
                m_pending.pop(channel, job_fn, job_data0, job_data1);
                uv_mutex_unlock(&m_mutex);

                // Execute outside lock
                if (job_fn)
                {
                    job_fn(job_data0, job_data1);
                }

                // Push into completed ring (may block if ring is full)
                uv_mutex_lock(&m_mutex);
                while (m_completed[channel].m_count == m_completed[channel].m_capacity)
                {
                    // Completed buffer full: wait until producer pops
                    // If stop with drop and no producer, this could block.
                    // We keep blocking to guarantee delivery of completed jobs.
                    uv_cond_wait(&m_room_completed[channel], &m_mutex);
                }
                if (job_fn)
                {
                    m_completed[channel].push(channel, job_fn, job_data0, job_data1);
                    uv_cond_signal(&m_has_completed[channel]);  // notify producer
                }
                // Loop back for next job
            }
        }
    };

    //------------------------------------------------------------------------------
    // Example job: prints a message with an integer payload.
    //------------------------------------------------------------------------------
    struct print_job_t : public job_t
    {
        i32   id;
        char* text;

        print_job_t(i32 job_id, char* msg)
            : id(job_id)
            , text(msg)
        {
        }
    };

    void print_job_fn(void* job_data0, void* job_data1)
    {
        print_job_t* data = (print_job_t*)job_data0;
        i32          id   = data->id;
        const char*  text = data->text;
        fprintf(stdout, "[Job %d] %s\n", id, (text ? text : "(null)"));
    }

    //------------------------------------------------------------------------------
    // Demo main: create manager, submit jobs, and pop completions.
    //------------------------------------------------------------------------------
    static i32 jobs_example()
    {
        alloc_t* allocator;

        // 4 maximum channels, 4 worker m_threads, pending ring capacity 32
        job_manager_t jm;
        jm.intialize(allocator, 4, 4, 32);

        // channel 0, completed ring capacity 32
        job_channel_t channel0 = jm.init_channel(&jm, 32);

        // Submit some jobs
        i32 i;
        for (i = 1; i <= 10; ++i)
        {
            // Persist message per job (allocate and keep until producer deletes job)
            print_job_t* job_data = (print_job_t*)malloc(sizeof(print_job_t));
            if (job_data)
            {
                job_data->text = (char*)malloc(64);

                const char* prefix = "Hello from libuv job manager";
                i32         k      = 0;
                while (prefix[k] != '\0' && k < 63)
                {
                    job_data->text[k] = prefix[k];
                    k++;
                }
                job_data->text[k] = '\0';

                i32 rc = jm.submit(channel0, print_job_fn, (void*)job_data, NULL);
                if (rc != 0)
                {
                    fprintf(stderr, "Submit failed for job %d\n", i);
                    free(job_data->text);
                    free(job_data);
                }
            }
            else
            {
                fprintf(stderr, "Failed to allocate message for job %d\n", i);
            }
        }

        // Pop completions (non-blocking loop with a simple retry)
        i32 completed_total = 0;
        for (;;)
        {
            void* done0 = NULL;
            void* done1 = NULL;
            i32   rc   = jm.pop_completed(channel0, done0, done1);
            if (rc == 0 && done0)
            {
                // In real code, inspect job result/state here.
                // Our demo job just prints in execute(); we now own deletion.
                print_job_t* pj = (print_job_t*)done0;
                if (pj && pj->text)
                {
                    free((void*)pj->text);
                    pj->text = NULL;
                }
                free((void*)pj);
                completed_total += 1;
                if (completed_total >= 10)
                {
                    break;
                }
            }
            else
            {
                // No completed jobs yet—sleep briefly
                uv_sleep(10);  // libuv utility to sleep milliseconds
            }
        }

        // Drain remaining pending (should be none) and stop
        jm.stop(/*drain=*/1);
        fprintf(stdout, "Completed %d jobs. Exiting.\n", completed_total);
        return 0;
    }

    job_manager_t* create_job_manager(alloc_t* allocator, i32 n_channels, i32 n_threads, i32 pending_capacity)
    {
        // Construct the job manager
        job_manager_t* jm = g_allocate<job_manager_t>(allocator);
        jm->intialize(allocator, n_channels, n_threads, pending_capacity);
        return jm;
    }

    void destroy_job_manager(job_manager_t*& manager)
    {
        if (manager)
        {
            alloc_t* allocator = manager->m_allocator;
            manager->shutdown();
            g_deallocate(allocator, manager);
            manager = nullptr;
        }
    }

    job_channel_t init_channel(job_manager_t* jm, i32 completed_capacity) { return jm->init_channel(jm, completed_capacity); }
    i32           push_job(job_manager_t* jm, job_channel_t channel, job_fn_t job_fn, void* job_data0, void* job_data1) { return jm->submit(channel, job_fn, job_data0, job_data1); }
    i32           pop_job(job_manager_t* jm, job_channel_t channel, void*& job_data0, void*& job_data1) { return jm->pop_completed(channel, job_data0, job_data1); }
    i32           pop_job_wait(job_manager_t* jm, job_channel_t channel, void*& job_data0, void*& job_data1) { return jm->pop_completed_wait(channel, job_data0, job_data1); }

}  // namespace ncore
