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

// Date: 2016/06/03 13:25:44

#include <gtest/gtest.h>
#include <flare/fiber/internal/countdown_event.h>
#include "flare/base/static_atomic.h"
#include "flare/base/time.h"

namespace {
struct Arg {
    flare::fiber_internal::CountdownEvent event;
    std::atomic<int> num_sig;
};

void *signaler(void *arg) {
    Arg* a = (Arg*)arg;
    a->num_sig.fetch_sub(1, std::memory_order_relaxed);
    a->event.signal();
    return NULL;
}

TEST(CountdonwEventTest, sanity) {
    for (int n = 1; n < 10; ++n) {
        Arg a;
        a.num_sig = n;
        a.event.reset(n);
        for (int i = 0; i < n; ++i) {
            fiber_id_t tid;
            ASSERT_EQ(0, fiber_start_urgent(&tid, NULL, signaler, &a));
        }
        a.event.wait();
        ASSERT_EQ(0, a.num_sig.load(std::memory_order_relaxed));
    }
}

TEST(CountdonwEventTest, timed_wait) {
    flare::fiber_internal::CountdownEvent event;
    int rc = event.timed_wait(flare::base::milliseconds_from_now(100));
    ASSERT_EQ(rc, ETIMEDOUT);
    event.signal();
    rc = event.timed_wait(flare::base::milliseconds_from_now(100));
    ASSERT_EQ(rc, 0);
    flare::fiber_internal::CountdownEvent event1;
    event1.signal();
    rc = event.timed_wait(flare::base::milliseconds_from_now(1));
    ASSERT_EQ(rc, 0);
}
} // namespace
