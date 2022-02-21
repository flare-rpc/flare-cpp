// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <inttypes.h>
#include <gtest/gtest.h>
#include "flare/base/compat.h"
#include "flare/base/time.h"
#include "flare/base/strings.h"
#include "flare/log/logging.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/waitable_event.h"
#include "flare/fiber/internal/schedule_group.h"
#include "flare/fiber/internal/mutex.h"
#include "flare/base/gperftools_profiler.h"
#include "flare/fiber/this_fiber.h"

namespace {
    inline unsigned *get_butex(fiber_mutex_t &m) {
        return m.event;
    }

    long start_time = flare::base::cpuwide_time_ms();
    int c = 0;

    void *locker(void *arg) {
        fiber_mutex_t *m = (fiber_mutex_t *) arg;
        fiber_mutex_lock(m);
        printf("[%" PRIu64 "] I'm here, %d, %" PRId64 "ms\n",
               pthread_numeric_id(), ++c, flare::base::cpuwide_time_ms() - start_time);
        flare::this_fiber::fiber_sleep_for(10000);
        fiber_mutex_unlock(m);
        return NULL;
    }

    TEST(MutexTest, sanity) {
        fiber_mutex_t m;
        ASSERT_EQ(0, fiber_mutex_init(&m, NULL));
        ASSERT_EQ(0u, *get_butex(m));
        ASSERT_EQ(0, fiber_mutex_lock(&m));
        ASSERT_EQ(1u, *get_butex(m));
        fiber_id_t th1;
        ASSERT_EQ(0, fiber_start_urgent(&th1, NULL, locker, &m));
        usleep(5000); // wait for locker to run.
        ASSERT_EQ(257u, *get_butex(m)); // contention
        ASSERT_EQ(0, fiber_mutex_unlock(&m));
        ASSERT_EQ(0, fiber_join(th1, NULL));
        ASSERT_EQ(0u, *get_butex(m));
        ASSERT_EQ(0, fiber_mutex_destroy(&m));
    }

    TEST(MutexTest, used_in_pthread) {
        fiber_mutex_t m;
        ASSERT_EQ(0, fiber_mutex_init(&m, NULL));
        pthread_t th[8];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], NULL, locker, &m));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            pthread_join(th[i], NULL);
        }
        ASSERT_EQ(0u, *get_butex(m));
        ASSERT_EQ(0, fiber_mutex_destroy(&m));
    }

    void *do_locks(void *arg) {
        struct timespec t = {-2, 0};
        EXPECT_EQ(ETIMEDOUT, fiber_mutex_timedlock((fiber_mutex_t *) arg, &t));
        return NULL;
    }

    TEST(MutexTest, timedlock) {
        fiber_cond_t c;
        fiber_mutex_t m1;
        fiber_mutex_t m2;
        ASSERT_EQ(0, fiber_cond_init(&c, NULL));
        ASSERT_EQ(0, fiber_mutex_init(&m1, NULL));
        ASSERT_EQ(0, fiber_mutex_init(&m2, NULL));

        struct timespec t = {-2, 0};

        fiber_mutex_lock(&m1);
        fiber_mutex_lock(&m2);
        fiber_id_t pth;
        ASSERT_EQ(0, fiber_start_urgent(&pth, NULL, do_locks, &m1));
        ASSERT_EQ(ETIMEDOUT, fiber_cond_timedwait(&c, &m2, &t));
        ASSERT_EQ(0, fiber_join(pth, NULL));
        fiber_mutex_unlock(&m1);
        fiber_mutex_unlock(&m2);
        fiber_mutex_destroy(&m1);
        fiber_mutex_destroy(&m2);
    }

    TEST(MutexTest, cpp_wrapper) {
        flare::fiber_internal::fiber_mutex mutex;
        ASSERT_TRUE(mutex.try_lock());
        mutex.unlock();
        mutex.lock();
        mutex.unlock();
        {
            FLARE_SCOPED_LOCK(mutex);
        }
        {
            std::unique_lock<flare::fiber_internal::fiber_mutex> lck1;
            std::unique_lock<flare::fiber_internal::fiber_mutex> lck2(mutex);
            lck1.swap(lck2);
            lck1.unlock();
            lck1.lock();
        }
        ASSERT_TRUE(mutex.try_lock());
        mutex.unlock();
        {
            FLARE_SCOPED_LOCK(*mutex.native_handler());
        }
        {
            std::unique_lock<fiber_mutex_t> lck1;
            std::unique_lock<fiber_mutex_t> lck2(*mutex.native_handler());
            lck1.swap(lck2);
            lck1.unlock();
            lck1.lock();
        }
        ASSERT_TRUE(mutex.try_lock());
        mutex.unlock();
    }

    bool g_started = false;
    bool g_stopped = false;

    template<typename fiber_mutex>
    struct FLARE_CACHELINE_ALIGNMENT PerfArgs {
        fiber_mutex *mutex;
        int64_t counter;
        int64_t elapse_ns;
        bool ready;

        PerfArgs() : mutex(NULL), counter(0), elapse_ns(0), ready(false) {}
    };

    template<typename fiber_mutex>
    void *add_with_mutex(void *void_arg) {
        PerfArgs<fiber_mutex> *args = (PerfArgs<fiber_mutex> *) void_arg;
        args->ready = true;
        flare::base::stop_watcher t;
        while (!g_stopped) {
            if (g_started) {
                break;
            }
            flare::this_fiber::fiber_sleep_for(1000);
        }
        t.start();
        while (!g_stopped) {
            FLARE_SCOPED_LOCK(*args->mutex);
            ++args->counter;
        }
        t.stop();
        args->elapse_ns = t.n_elapsed();
        return NULL;
    }

    int g_prof_name_counter = 0;

    template<typename fiber_mutex, typename ThreadId,
            typename ThreadCreateFn, typename ThreadJoinFn>
    void PerfTest(fiber_mutex *mutex,
                  ThreadId * /*dummy*/,
                  int thread_num,
                  const ThreadCreateFn &create_fn,
                  const ThreadJoinFn &join_fn) {
        g_started = false;
        g_stopped = false;
        ThreadId threads[thread_num];
        std::vector<PerfArgs<fiber_mutex> > args(thread_num);
        for (int i = 0; i < thread_num; ++i) {
            args[i].mutex = mutex;
            create_fn(&threads[i], NULL, add_with_mutex<fiber_mutex>, &args[i]);
        }
        while (true) {
            bool all_ready = true;
            for (int i = 0; i < thread_num; ++i) {
                if (!args[i].ready) {
                    all_ready = false;
                    break;
                }
            }
            if (all_ready) {
                break;
            }
            usleep(1000);
        }
        g_started = true;
        char prof_name[32];
        snprintf(prof_name, sizeof(prof_name), "mutex_perf_%d.prof", ++g_prof_name_counter);
        ProfilerStart(prof_name);
        usleep(500 * 1000);
        ProfilerStop();
        g_stopped = true;
        int64_t wait_time = 0;
        int64_t count = 0;
        for (int i = 0; i < thread_num; ++i) {
            join_fn(threads[i], NULL);
            wait_time += args[i].elapse_ns;
            count += args[i].counter;
        }
        LOG(INFO) << flare::base::class_name<fiber_mutex>() << " in "
                  << ((void *) create_fn == (void *) pthread_create ? "pthread" : "fiber")
                  << " thread_num=" << thread_num
                  << " count=" << count
                  << " average_time=" << wait_time / (double) count;
    }

    TEST(MutexTest, performance) {
        const int thread_num = 12;
        flare::base::Mutex base_mutex;
        PerfTest(&base_mutex, (pthread_t *) NULL, thread_num, pthread_create, pthread_join);
        PerfTest(&base_mutex, (fiber_id_t *) NULL, thread_num, fiber_start_background, fiber_join);
        flare::fiber_internal::fiber_mutex bth_mutex;
        PerfTest(&bth_mutex, (pthread_t *) NULL, thread_num, pthread_create, pthread_join);
        PerfTest(&bth_mutex, (fiber_id_t *) NULL, thread_num, fiber_start_background, fiber_join);
    }

    void *loop_until_stopped(void *arg) {
        flare::fiber_internal::fiber_mutex *m = (flare::fiber_internal::fiber_mutex *) arg;
        while (!g_stopped) {
            FLARE_SCOPED_LOCK(*m);
            flare::this_fiber::fiber_sleep_for(20);
        }
        return NULL;
    }

    TEST(MutexTest, mix_thread_types) {
        g_stopped = false;
        const int N = 16;
        const int M = N * 2;
        flare::fiber_internal::fiber_mutex m;
        pthread_t pthreads[N];
        fiber_id_t fibers[M];
        // reserve enough workers for test. This is a must since we have
        // FIBER_ATTR_PTHREAD fibers which may cause deadlocks (the
        // bhtread_usleep below can't be scheduled and g_stopped is never
        // true, thus loop_until_stopped spins forever)
        fiber_setconcurrency(M);
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(0, pthread_create(&pthreads[i], NULL, loop_until_stopped, &m));
        }
        for (int i = 0; i < M; ++i) {
            const fiber_attribute *attr = i % 2 ? NULL : &FIBER_ATTR_PTHREAD;
            ASSERT_EQ(0, fiber_start_urgent(&fibers[i], attr, loop_until_stopped, &m));
        }
        flare::this_fiber::fiber_sleep_for(1000L * 1000);
        g_stopped = true;
        for (int i = 0; i < M; ++i) {
            fiber_join(fibers[i], NULL);
        }
        for (int i = 0; i < N; ++i) {
            pthread_join(pthreads[i], NULL);
        }
    }
} // namespace