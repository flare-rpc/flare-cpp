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
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "testing/gtest_wrap.h"
#include "flare/times/time.h"
#include "flare/base/errno.h"
#include <limits.h>                            // INT_MAX
#include "flare/base/static_atomic.h"
#include "flare/fiber/internal/fiber.h"
#include <flare/fiber/internal/sys_futex.h>
#include <flare/fiber/internal/processor.h>
#include "flare/fiber/this_fiber.h"

namespace {
    volatile bool stop = false;

    std::atomic<int> nthread(0);

    void *read_thread(void *arg) {
        std::atomic<int> *m = (std::atomic<int> *) arg;
        int njob = 0;
        while (!stop) {
            int x;
            while (!stop && (x = *m) != 0) {
                if (x > 0) {
                    while ((x = m->fetch_sub(1)) > 0) {
                        ++njob;
                        const long start = flare::get_current_time_nanos();
                        while (flare::get_current_time_nanos() < start + 10000) {
                        }
                        if (stop) {
                            return new int(njob);
                        }
                    }
                    m->fetch_add(1);
                } else {
                    cpu_relax();
                }
            }

            ++nthread;
            flare::fiber_internal::futex_wait_private(m/*lock1*/, 0/*consumed_njob*/, nullptr);
            --nthread;
        }
        return new int(njob);
    }

    TEST(FutexTest, rdlock_performance) {
        const size_t N = 100000;
        std::atomic<int> lock1(0);
        pthread_t rth[8];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(rth); ++i) {
            ASSERT_EQ(0, pthread_create(&rth[i], nullptr, read_thread, &lock1));
        }

        const int64_t t1 = flare::get_current_time_nanos();
        for (size_t i = 0; i < N; ++i) {
            if (nthread) {
                lock1.fetch_add(1);
                flare::fiber_internal::futex_wake_private(&lock1, 1);
            } else {
                lock1.fetch_add(1);
                if (nthread) {
                    flare::fiber_internal::futex_wake_private(&lock1, 1);
                }
            }
        }
        const int64_t t2 = flare::get_current_time_nanos();

        flare::fiber_sleep_for(3000000);
        stop = true;
        for (int i = 0; i < 10; ++i) {
            flare::fiber_internal::futex_wake_private(&lock1, INT_MAX);
            sched_yield();
        }

        int njob = 0;
        int *res;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(rth); ++i) {
            pthread_join(rth[i], (void **) &res);
            njob += *res;
            delete res;
        }
        printf("wake %lu times, %" PRId64 "ns each, lock1=%d njob=%d\n",
               N, (t2 - t1) / N, lock1.load(), njob);
        ASSERT_EQ(N, (size_t) (lock1.load() + njob));
    }

    TEST(FutexTest, futex_wake_before_wait) {
        int lock1 = 0;
        timespec timeout = {1, 0};
        ASSERT_EQ(0, flare::fiber_internal::futex_wake_private(&lock1, INT_MAX));
        ASSERT_EQ(-1, flare::fiber_internal::futex_wait_private(&lock1, 0, &timeout));
        ASSERT_EQ(ETIMEDOUT, errno);
    }

    void *dummy_waiter(void *lock) {
        flare::fiber_internal::futex_wait_private(lock, 0, nullptr);
        return nullptr;
    }

    TEST(FutexTest, futex_wake_many_waiters_perf) {

        int lock1 = 0;
        size_t N = 0;
        pthread_t th;
        for (; N < 1000 && !pthread_create(&th, nullptr, dummy_waiter, &lock1); ++N) {}

        sleep(1);
        int nwakeup = 0;
        flare::stop_watcher tm;
        tm.start();
        for (size_t i = 0; i < N; ++i) {
            nwakeup += flare::fiber_internal::futex_wake_private(&lock1, 1);
        }
        tm.stop();
        printf("N=%lu, futex_wake a thread = %" PRId64 "ns\n", N, tm.n_elapsed() / N);
        ASSERT_EQ(N, (size_t) nwakeup);

        sleep(2);
        const size_t REP = 10000;
        nwakeup = 0;
        tm.start();
        for (size_t i = 0; i < REP; ++i) {
            nwakeup += flare::fiber_internal::futex_wake_private(&lock1, 1);
        }
        tm.stop();
        ASSERT_EQ(0, nwakeup);
        printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
    }

    std::atomic<int> nevent(0);

    void *waker(void *lock) {
        flare::fiber_sleep_for(10000);
        const size_t REP = 100000;
        int nwakeup = 0;
        flare::stop_watcher tm;
        tm.start();
        for (size_t i = 0; i < REP; ++i) {
            nwakeup += flare::fiber_internal::futex_wake_private(lock, 1);
        }
        tm.stop();
        EXPECT_EQ(0, nwakeup);
        printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
        return nullptr;
    }

    void *batch_waker(void *lock) {
        flare::fiber_sleep_for(10000);
        const size_t REP = 100000;
        int nwakeup = 0;
        flare::stop_watcher tm;
        tm.start();
        for (size_t i = 0; i < REP; ++i) {
            if (nevent.fetch_add(1, std::memory_order_relaxed) == 0) {
                nwakeup += flare::fiber_internal::futex_wake_private(lock, 1);
                int expected = 1;
                while (1) {
                    int last_expected = expected;
                    if (nevent.compare_exchange_strong(expected, 0, std::memory_order_relaxed)) {
                        break;
                    }
                    nwakeup += flare::fiber_internal::futex_wake_private(lock, expected - last_expected);
                }
            }
        }
        tm.stop();
        EXPECT_EQ(0, nwakeup);
        printf("futex_wake nop = %" PRId64 "ns\n", tm.n_elapsed() / REP);
        return nullptr;
    }

    TEST(FutexTest, many_futex_wake_nop_perf) {
        pthread_t th[8];
        int lock1;
        std::cout << "[Direct wake]" << std::endl;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, waker, &lock1));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], nullptr));
        }
        std::cout << "[Batch wake]" << std::endl;
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_create(&th[i], nullptr, batch_waker, &lock1));
        }
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(th); ++i) {
            ASSERT_EQ(0, pthread_join(th[i], nullptr));
        }
    }
} // namespace
