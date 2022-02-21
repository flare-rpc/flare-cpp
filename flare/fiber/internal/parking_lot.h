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

// Date: 2017/07/27 23:07:06

#ifndef FLARE_FIBER_INTERNAL_PARKING_LOT_H_
#define FLARE_FIBER_INTERNAL_PARKING_LOT_H_

#include "flare/base/static_atomic.h"
#include "flare/fiber/internal/sys_futex.h"

namespace flare::fiber_internal {

// Park idle workers.
    class FLARE_CACHELINE_ALIGNMENT ParkingLot {
    public:
        class State {
        public:
            State() : val(0) {}

            bool stopped() const { return val & 1; }

        private:

            friend class ParkingLot;

            State(int val) : val(val) {}

            int val;
        };

        ParkingLot() : _pending_signal(0) {}

        // Wake up at most `num_task' workers.
        // Returns #workers woken up.
        int signal(int num_task) {
            _pending_signal.fetch_add((num_task << 1), std::memory_order_release);
            return futex_wake_private(&_pending_signal, num_task);
        }

        // Get a state for later wait().
        State get_state() {
            return _pending_signal.load(std::memory_order_acquire);
        }

        // Wait for tasks.
        // If the `expected_state' does not match, wait() may finish directly.
        void wait(const State &expected_state) {
            futex_wait_private(&_pending_signal, expected_state.val, NULL);
        }

        // Wakeup suspended wait() and make them unwaitable ever.
        void stop() {
            _pending_signal.fetch_or(1);
            futex_wake_private(&_pending_signal, 10000);
        }

    private:
        // higher 31 bits for signalling, LSB for stopping.
        std::atomic<int> _pending_signal;
    };

}  // namespace flare::fiber_internal

#endif  // FLARE_FIBER_INTERNAL_PARKING_LOT_H_
