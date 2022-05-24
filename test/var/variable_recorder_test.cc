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

// Date 2014/10/13 19:47:59

#include "testing/gtest_wrap.h"
#include <pthread.h>                                // pthread_*

#include <cstddef>
#include <memory>
#include <iostream>
#include "flare/times/time.h"
#include "flare/variable/recorder.h"
#include "flare/variable/latency_recorder.h"
#include "flare/strings/str_join.h"

namespace {
TEST(RecorderTest, test_complement) {
    FLARE_LOG(INFO) << "sizeof(LatencyRecorder)=" << sizeof(flare::variable::LatencyRecorder)
              << " " << sizeof(flare::variable::detail::Percentile)
              << " " << sizeof(flare::variable::Maxer<int64_t>)
              << " " << sizeof(flare::variable::IntRecorder)
              << " " << sizeof(flare::variable::Window<flare::variable::IntRecorder>)
              << " " << sizeof(flare::variable::Window<flare::variable::detail::Percentile>);

    for (int a = -10000000; a < 10000000; ++a) {
        const uint64_t complement = flare::variable::IntRecorder::_get_complement(a);
        const int64_t b = flare::variable::IntRecorder::_extend_sign_bit(complement);
        ASSERT_EQ(a, b);
    }
}

TEST(RecorderTest, test_compress) {
    const uint64_t num = 125345;
    const uint64_t sum = 26032906;
    const uint64_t compressed = flare::variable::IntRecorder::_compress(num, sum);
    ASSERT_EQ(num, flare::variable::IntRecorder::_get_num(compressed));
    ASSERT_EQ(sum, flare::variable::IntRecorder::_get_sum(compressed));
}

TEST(RecorderTest, test_compress_negtive_number) {
    for (int a = -10000000; a < 10000000; ++a) {
        const uint64_t sum = flare::variable::IntRecorder::_get_complement(a);
        const uint64_t num = 123456;
        const uint64_t compressed = flare::variable::IntRecorder::_compress(num, sum);
        ASSERT_EQ(num, flare::variable::IntRecorder::_get_num(compressed));
        ASSERT_EQ(a, flare::variable::IntRecorder::_extend_sign_bit(flare::variable::IntRecorder::_get_sum(compressed)));
    }
}

TEST(RecorderTest, sanity) {
    {
        flare::variable::IntRecorder recorder;
        ASSERT_TRUE(recorder.valid());
        ASSERT_EQ(0, recorder.expose("var1"));
        for (size_t i = 0; i < 100; ++i) {
            recorder << 2;
        }
        ASSERT_EQ(2l, (int64_t)recorder.average());
        ASSERT_EQ("2", flare::variable::Variable::describe_exposed("var1"));
        std::vector<std::string> vars;
        flare::variable::Variable::list_exposed(&vars);
        ASSERT_EQ(1UL, vars.size())<<flare::string_join(vars, ",");
        ASSERT_EQ("var1", vars[0]);
        ASSERT_EQ(1UL, flare::variable::Variable::count_exposed());
    }
    ASSERT_EQ(0UL, flare::variable::Variable::count_exposed());
}

TEST(RecorderTest, window) {
    flare::variable::IntRecorder c1;
    ASSERT_TRUE(c1.valid());
    flare::variable::Window<flare::variable::IntRecorder> w1(&c1, 1);
    flare::variable::Window<flare::variable::IntRecorder> w2(&c1, 2);
    flare::variable::Window<flare::variable::IntRecorder> w3(&c1, 3);

    const int N = 10000;
    int64_t last_time = flare::get_current_time_micros();
    for (int i = 1; i <= N; ++i) {
        c1 << i;
        int64_t now = flare::get_current_time_micros();
        if (now - last_time >= 1000000L) {
            last_time = now;
            FLARE_LOG(INFO) << "c1=" << c1 << " w1=" << w1 << " w2=" << w2 << " w3=" << w3;
        } else {
            usleep(950);
        }
    }
}

TEST(RecorderTest, negative) {
    flare::variable::IntRecorder recorder;
    ASSERT_TRUE(recorder.valid());
    for (size_t i = 0; i < 3; ++i) {
        recorder << -2;
    }
    ASSERT_EQ(-2, recorder.average());
}

TEST(RecorderTest, positive_overflow) {
    flare::variable::IntRecorder recorder1;
    ASSERT_TRUE(recorder1.valid());
    for (int i = 0; i < 5; ++i) {
        recorder1 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder1.average());

    flare::variable::IntRecorder recorder2;
    ASSERT_TRUE(recorder2.valid());
    recorder2.set_debug_name("recorder2");
    for (int i = 0; i < 5; ++i) {
        recorder2 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder2.average());

    flare::variable::IntRecorder recorder3;
    ASSERT_TRUE(recorder3.valid());
    recorder3.expose("recorder3");
    for (int i = 0; i < 5; ++i) {
        recorder3 << std::numeric_limits<int64_t>::max();
    }
    ASSERT_EQ(std::numeric_limits<int>::max(), recorder3.average());

    flare::variable::LatencyRecorder latency1;
    latency1.expose("latency1");
    latency1 << std::numeric_limits<int64_t>::max();

    flare::variable::LatencyRecorder latency2;
    latency2 << std::numeric_limits<int64_t>::max();
}

TEST(RecorderTest, negtive_overflow) {
    flare::variable::IntRecorder recorder1;
    ASSERT_TRUE(recorder1.valid());
    for (int i = 0; i < 5; ++i) {
        recorder1 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder1.average());

    flare::variable::IntRecorder recorder2;
    ASSERT_TRUE(recorder2.valid());
    recorder2.set_debug_name("recorder2");
    for (int i = 0; i < 5; ++i) {
        recorder2 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder2.average());

    flare::variable::IntRecorder recorder3;
    ASSERT_TRUE(recorder3.valid());
    recorder3.expose("recorder3");
    for (int i = 0; i < 5; ++i) {
        recorder3 << std::numeric_limits<int64_t>::min();
    }
    ASSERT_EQ(std::numeric_limits<int>::min(), recorder3.average());

    flare::variable::LatencyRecorder latency1;
    latency1.expose("latency1");
    latency1 << std::numeric_limits<int64_t>::min();

    flare::variable::LatencyRecorder latency2;
    latency2 << std::numeric_limits<int64_t>::min();
}

const size_t OPS_PER_THREAD = 20000000;

static void *thread_counter(void *arg) {
    flare::variable::IntRecorder *recorder = (flare::variable::IntRecorder *)arg;
    flare::stop_watcher timer;
    timer.start();
    for (int i = 0; i < (int)OPS_PER_THREAD; ++i) {
        *recorder << i;
    }
    timer.stop();
    return (void *)(timer.n_elapsed());
}

TEST(RecorderTest, perf) {
    flare::variable::IntRecorder recorder;
    ASSERT_TRUE(recorder.valid());
    pthread_t threads[8];
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        pthread_create(&threads[i], NULL, &thread_counter, (void *)&recorder);
    }
    long totol_time = 0;
    for (size_t i = 0; i < FLARE_ARRAY_SIZE(threads); ++i) {
        void *ret; 
        pthread_join(threads[i], &ret);
        totol_time += (long)ret;
    }
    ASSERT_EQ(((int64_t)OPS_PER_THREAD - 1) / 2, recorder.average());
    FLARE_LOG(INFO) << "Recorder takes " << totol_time / (OPS_PER_THREAD * FLARE_ARRAY_SIZE(threads))
              << "ns per sample with " << FLARE_ARRAY_SIZE(threads)
              << " threads";
}
} // namespace
