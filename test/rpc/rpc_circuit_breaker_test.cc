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



// Date: 2018/09/19 14:51:06

#include <pthread.h>
#include "testing/gtest_wrap.h"
#include <gflags/gflags.h>
#include "flare/fiber/internal/fiber.h"
#include "flare/rpc/circuit_breaker.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/server.h"
#include "echo.pb.h"

namespace {
void initialize_random() {
    srand(time(0));
}

const int kShortWindowSize = 500;
const int kLongWindowSize = 1000;
const int kShortWindowErrorPercent = 10;
const int kLongWindowErrorPercent = 5;
const int kMinIsolationDurationMs = 10;
const int kMaxIsolationDurationMs = 200;
const int kErrorCodeForFailed = 131;
const int kErrorCodeForSucc = 0;
const int kErrorCost = 1000;
const int kLatency = 1000;
const int kThreadNum = 3;
} // namespace

namespace flare::rpc {
DECLARE_int32(circuit_breaker_short_window_size);
DECLARE_int32(circuit_breaker_long_window_size);
DECLARE_int32(circuit_breaker_short_window_error_percent);
DECLARE_int32(circuit_breaker_long_window_error_percent);
DECLARE_int32(circuit_breaker_min_isolation_duration_ms);
DECLARE_int32(circuit_breaker_max_isolation_duration_ms);
} // namespace flare::rpc

int main(int argc, char* argv[]) {
    flare::rpc::FLAGS_circuit_breaker_short_window_size = kShortWindowSize;
    flare::rpc::FLAGS_circuit_breaker_long_window_size = kLongWindowSize;
    flare::rpc::FLAGS_circuit_breaker_short_window_error_percent = kShortWindowErrorPercent;
    flare::rpc::FLAGS_circuit_breaker_long_window_error_percent = kLongWindowErrorPercent;
    flare::rpc::FLAGS_circuit_breaker_min_isolation_duration_ms = kMinIsolationDurationMs;
    flare::rpc::FLAGS_circuit_breaker_max_isolation_duration_ms = kMaxIsolationDurationMs;
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

pthread_once_t initialize_random_control = PTHREAD_ONCE_INIT;

struct FeedbackControl {
    FeedbackControl(int req_num, int error_percent,
                 flare::rpc::CircuitBreaker* circuit_breaker)
        : _req_num(req_num)
        , _error_percent(error_percent)
        , _circuit_breaker(circuit_breaker)
        , _healthy_cnt(0)
        , _unhealthy_cnt(0)
        , _healthy(true) {}
    int _req_num;
    int _error_percent;
    flare::rpc::CircuitBreaker* _circuit_breaker;
    int _healthy_cnt;
    int _unhealthy_cnt;
    bool _healthy;
};

class CircuitBreakerTest : public ::testing::Test {
protected:
    CircuitBreakerTest() {
        pthread_once(&initialize_random_control, initialize_random);
    };

    virtual ~CircuitBreakerTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    static void* feed_back_thread(void* data) {
        FeedbackControl* fc = static_cast<FeedbackControl*>(data);
        for (int i = 0; i < fc->_req_num; ++i) {
            bool healthy = false;
            if (rand() % 100 < fc->_error_percent) {
                healthy = fc->_circuit_breaker->OnCallEnd(kErrorCodeForFailed, kErrorCost);
            } else {
                healthy = fc->_circuit_breaker->OnCallEnd(kErrorCodeForSucc, kLatency);
            }
            fc->_healthy = healthy;
            if (healthy) {
                ++fc->_healthy_cnt;
            } else {
                ++fc->_unhealthy_cnt;
            }
        }
        return fc;
    }

    void StartFeedbackThread(std::vector<pthread_t>* thread_list,
                             std::vector<std::unique_ptr<FeedbackControl>>* fc_list,
                             int error_percent) {
        thread_list->clear();
        fc_list->clear();
        for (int i = 0; i < kThreadNum; ++i) {
            pthread_t tid = 0;
            FeedbackControl* fc =
                new FeedbackControl(2 * kLongWindowSize, error_percent, &_circuit_breaker);
            fc_list->emplace_back(fc);
            pthread_create(&tid, nullptr, feed_back_thread, fc);
            thread_list->push_back(tid);
        }
    }

    flare::rpc::CircuitBreaker _circuit_breaker;
};

TEST_F(CircuitBreakerTest, should_not_isolate) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 3);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_EQ(fc->_unhealthy_cnt, 0);
        EXPECT_TRUE(fc->_healthy);
    }
}

TEST_F(CircuitBreakerTest, should_isolate) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 50);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
        EXPECT_FALSE(fc->_healthy);
    }
}

TEST_F(CircuitBreakerTest, isolation_duration_grow_and_reset) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs);

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs * 2);

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs * 4);

    _circuit_breaker.Reset();
    ::usleep((kMaxIsolationDurationMs + kMinIsolationDurationMs) * 1000);
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs);

}

TEST_F(CircuitBreakerTest, maximum_isolation_duration) {
    flare::rpc::FLAGS_circuit_breaker_max_isolation_duration_ms =
        flare::rpc::FLAGS_circuit_breaker_min_isolation_duration_ms + 1;
    ASSERT_LT(flare::rpc::FLAGS_circuit_breaker_max_isolation_duration_ms,
              2 * flare::rpc::FLAGS_circuit_breaker_min_isolation_duration_ms);
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(),
              flare::rpc::FLAGS_circuit_breaker_max_isolation_duration_ms);
}
