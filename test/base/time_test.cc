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
#include "flare/base/profile.h"
/*
#if defined(FLARE_PLATFORM_LINUX)
#include <syscall.h>                         // SYS_clock_gettime
#include <unistd.h>                          // syscall
#endif

#include "flare/times/time.h"
#include "flare/log/logging.h"

namespace {

    TEST(BaiduTimeTest, diff_between_gettimeofday_and_REALTIME) {
        long t1 = flare::get_current_time_micros();
        timespec time;
        clock_gettime(CLOCK_REALTIME, &time);
        long t2 = flare::base::timespec_to_microseconds(time);
        LOG(INFO) << "t1=" << t1 << " t2=" << t2;
    }

    const char *clock_desc[] = {
            "CLOCK_REALTIME",                 //0
            "CLOCK_MONOTONIC",                //1
            "CLOCK_PROCESS_CPUTIME_ID",       //2
            "CLOCK_THREAD_CPUTIME_ID",        //3
            "CLOCK_MONOTONIC_RAW",            //4
            "CLOCK_REALTIME_COARSE",          //5
            "CLOCK_MONOTONIC_COARSE",         //6
            "CLOCK_BOOTTIME",                 //7
            "CLOCK_REALTIME_ALARM",           //8
            "CLOCK_BOOTTIME_ALARM",           //9
            "CLOCK_SGI_CYCLE",                //10
            "CLOCK_TAI"                       //11
    };

    TEST(BaiduTimeTest, cost_of_timer) {
        printf("sizeof(time_t)=%lu\n", sizeof(time_t));

        flare::stop_watcher t1, t2;
        timespec ts;
        const size_t N = 200000;
        t1.start();
        for (size_t i = 0; i < N; ++i) {
            t2.stop();
        }
        t1.stop();
        printf("Timer::stop() takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        t1.start();
        for (size_t i = 0; i < N; ++i) {
            clock();
        }
        t1.stop();
        printf("clock() takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        long s = 0;
        t1.start();
        for (size_t i = 0; i < N; ++i) {
            s += flare::get_current_time_nanos();
        }
        t1.stop();
        printf("cpuwide_time() takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        t1.start();
        for (size_t i = 0; i < N; ++i) {
            s += flare::get_current_time_micros();
        }
        t1.stop();
        printf("gettimeofday_us takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        t1.start();
        for (size_t i = 0; i < N; ++i) {
            time(NULL);
        }
        t1.stop();
        printf("time(NULL) takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        t1.start();
        for (size_t i = 0; i < N; ++i) {
            s += flare::base::monotonic_time_ns();
        }
        t1.stop();
        printf("monotonic_time_ns takes %" PRId64 "ns\n", t1.n_elapsed() / N);

        for (size_t i = 0; i < FLARE_ARRAY_SIZE(clock_desc); ++i) {
#if defined(FLARE_PLATFORM_LINUX)
            if (0 == syscall(SYS_clock_gettime, (clockid_t)i, &ts)) {
                t1.start();
                for (size_t j = 0; j < N; ++j) {
                    syscall(SYS_clock_gettime, (clockid_t)i, &ts);
                }
                t1.stop();
                printf("sys   clock_gettime(%s) takes %" PRId64 "ns\n",
                       clock_desc[i], t1.n_elapsed() / N);
            }
#endif
            if (0 == clock_gettime((clockid_t) i, &ts)) {
                t1.start();
                for (size_t j = 0; j < N; ++j) {
                    clock_gettime((clockid_t) i, &ts);
                }
                t1.stop();
                printf("glibc clock_gettime(%s) takes %" PRId64 "ns\n",
                       clock_desc[i], t1.n_elapsed() / N);
            }
        }
    }

    TEST(BaiduTimeTest, timespec) {
        timespec ts1 = {0, -1};
        flare::base::timespec_normalize(&ts1);
        ASSERT_EQ(999999999L, ts1.tv_nsec);
        ASSERT_EQ(-1, ts1.tv_sec);

        timespec ts2 = {0, 1000000000L};
        flare::base::timespec_normalize(&ts2);
        ASSERT_EQ(0L, ts2.tv_nsec);
        ASSERT_EQ(1L, ts2.tv_sec);

        timespec ts3 = {0, 999999999L};
        flare::base::timespec_normalize(&ts3);
        ASSERT_EQ(999999999L, ts3.tv_nsec);
        ASSERT_EQ(0, ts3.tv_sec);

        timespec ts4 = {0, 1L};
        flare::base::timespec_add(&ts4, ts3);
        ASSERT_EQ(0, ts4.tv_nsec);
        ASSERT_EQ(1L, ts4.tv_sec);

        timespec ts5 = {0, 999999999L};
        flare::base::timespec_minus(&ts5, ts3);
        ASSERT_EQ(0, ts5.tv_nsec);
        ASSERT_EQ(0, ts5.tv_sec);

        timespec ts6 = {0, 999999998L};
        flare::base::timespec_minus(&ts6, ts3);
        ASSERT_EQ(999999999L, ts6.tv_nsec);
        ASSERT_EQ(-1L, ts6.tv_sec);

        timespec ts7 = flare::base::nanoseconds_from(ts3, 1L);
        ASSERT_EQ(0, ts7.tv_nsec);
        ASSERT_EQ(1L, ts7.tv_sec);

        timespec ts8 = flare::base::nanoseconds_from(ts3, -1000000000L);
        ASSERT_EQ(999999999L, ts8.tv_nsec);
        ASSERT_EQ(-1L, ts8.tv_sec);

        timespec ts9 = flare::base::microseconds_from(ts3, 1L);
        ASSERT_EQ(999L, ts9.tv_nsec);
        ASSERT_EQ(1L, ts9.tv_sec);

        timespec ts10 = flare::base::microseconds_from(ts3, -1000000L);
        ASSERT_EQ(999999999L, ts10.tv_nsec);
        ASSERT_EQ(-1L, ts10.tv_sec);

        timespec ts11 = flare::base::milliseconds_from(ts3, 1L);
        ASSERT_EQ(999999L, ts11.tv_nsec);
        ASSERT_EQ(1L, ts11.tv_sec);

        timespec ts12 = flare::base::milliseconds_from(ts3, -1000L);
        ASSERT_EQ(999999999L, ts12.tv_nsec);
        ASSERT_EQ(-1L, ts12.tv_sec);

        timespec ts13 = flare::base::seconds_from(ts3, 1L);
        ASSERT_EQ(999999999L, ts13.tv_nsec);
        ASSERT_EQ(1, ts13.tv_sec);

        timespec ts14 = flare::base::seconds_from(ts3, -1L);
        ASSERT_EQ(999999999L, ts14.tv_nsec);
        ASSERT_EQ(-1L, ts14.tv_sec);
    }

    TEST(BaiduTimeTest, every_many_us) {
        flare::every_duration every_10ms(flare::duration::milliseconds(10));
        size_t i = 0;
        const long start_time = flare::time_now().to_unix_millis();
        while (1) {
            if (every_10ms) {
                printf("enter this branch at %" PRId64 "ms\n",
                       flare::time_now().to_unix_millis() - start_time);
                if (++i >= 10) {
                    break;
                }
            }
        }
    }

    TEST(BaiduTimeTest, timer_auto_start) {
        flare::stop_watcher t(flare::stop_watcher::STARTED);
        usleep(100);
        t.stop();
        printf("Cost %" PRId64 "us\n", t.u_elapsed());
    }

} // namespace
*/