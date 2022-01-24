// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/butil/threading/thread_collision_warner.h"

#include "flare/base/logging.h"
#include "flare/butil/threading/platform_thread.h"

namespace butil {

    void DCheckAsserter::warn() {
        NOTREACHED() << "Thread Collision";
    }

    static int32_t CurrentThread() {
        const PlatformThreadId current_thread_id = PlatformThread::CurrentId();
        // We need to get the thread id into an atomic data type. This might be a
        // truncating conversion, but any loss-of-information just increases the
        // chance of a fault negative, not a false positive.
        const int32_t atomic_thread_id =
                static_cast<int32_t>(current_thread_id);

        return atomic_thread_id;
    }

    void ThreadCollisionWarner::EnterSelf() {
        // If the active thread is 0 then I'll write the current thread ID
        // if two or more threads arrive here only one will succeed to
        // write on valid_thread_id_ the current thread ID.
        int32_t current_thread_id = CurrentThread();

        int previous_value = 0;
        auto r = valid_thread_id_.compare_exchange_weak(previous_value, current_thread_id, std::memory_order_release);
        if (previous_value != 0 && previous_value != current_thread_id) {
            // gotcha! a thread is trying to use the same class and that is
            // not current thread.
            asserter_->warn();
        }

        counter_.fetch_add(1);
    }

    void ThreadCollisionWarner::Enter() {
        int32_t current_thread_id = CurrentThread();

        int32_t old_value = 0;
        valid_thread_id_.compare_exchange_weak(old_value, current_thread_id);
        if (old_value != 0) {
            // gotcha! another thread is trying to use the same class.
            asserter_->warn();
        }

        counter_.fetch_add(1);
    }

    void ThreadCollisionWarner::Leave() {
        if (counter_.fetch_add(-1) == 1) {
            valid_thread_id_.store(0);
        }
    }

}  // namespace butil
