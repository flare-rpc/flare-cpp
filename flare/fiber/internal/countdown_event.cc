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

// fiber - A M:N threading library to make applications more concurrent.

// Date: 2016/06/03 13:15:24

#include "flare/base/static_atomic.h"     // std::atomic<int>
#include "flare/fiber/internal/waitable_event.h"
#include "flare/fiber/internal/countdown_event.h"

namespace flare::fiber_internal {

CountdownEvent::CountdownEvent(int initial_count) {
    if (initial_count < 0) {
        LOG(FATAL) << "Invalid initial_count=" << initial_count;
        abort();
    }
    _butex = waitable_event_create_checked<int>();
    *_butex = initial_count;
    _wait_was_invoked = false;
}

CountdownEvent::~CountdownEvent() {
    waitable_event_destroy(_butex);
}

void CountdownEvent::signal(int sig) {
    // Have to save _butex, *this is probably defreferenced by the wait thread
    // which sees fetch_sub
    void* const saved_butex = _butex;
    const int prev = ((std::atomic<int>*)_butex)
        ->fetch_sub(sig, std::memory_order_release);
    // DON'T touch *this ever after
    if (prev > sig) {
        return;
    }
    LOG_IF(ERROR, prev < sig) << "Counter is over decreased";
    waitable_event_wake_all(saved_butex);
}

int CountdownEvent::wait() {
    _wait_was_invoked = true;
    for (;;) {
        const int seen_counter = 
            ((std::atomic<int>*)_butex)->load(std::memory_order_acquire);
        if (seen_counter <= 0) {
            return 0;
        }
        if (waitable_event_wait(_butex, seen_counter, NULL) < 0 &&
            errno != EWOULDBLOCK && errno != EINTR) {
            return errno;
        }
    }
}

void CountdownEvent::add_count(int v) {
    if (v <= 0) {
        LOG_IF(ERROR, v < 0) << "Invalid count=" << v;
        return;
    }
    LOG_IF(ERROR, _wait_was_invoked) 
            << "Invoking add_count() after wait() was invoked";
    ((std::atomic<int>*)_butex)->fetch_add(v, std::memory_order_release);
}

void CountdownEvent::reset(int v) {
    if (v < 0) {
        LOG(ERROR) << "Invalid count=" << v;
        return;
    }
    const int prev_counter =
            ((std::atomic<int>*)_butex)
                ->exchange(v, std::memory_order_release);
    LOG_IF(ERROR, _wait_was_invoked && prev_counter)
        << "Invoking reset() while count=" << prev_counter;
    _wait_was_invoked = false;
}

int CountdownEvent::timed_wait(const timespec& duetime) {
    _wait_was_invoked = true;
    for (;;) {
        const int seen_counter = 
            ((std::atomic<int>*)_butex)->load(std::memory_order_acquire);
        if (seen_counter <= 0) {
            return 0;
        }
        if (waitable_event_wait(_butex, seen_counter, &duetime) < 0 &&
            errno != EWOULDBLOCK && errno != EINTR) {
            return errno;
        }
    }
}

}  // namespace flare::fiber_internal
