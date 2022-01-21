// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/butil/memory/singleton.h"
#include "flare/butil/threading/platform_thread.h"

namespace butil {
    namespace internal {

        intptr_t WaitForInstance(std::atomic<intptr_t> *instance) {
            // Handle the race. Another thread beat us and either:
            // - Has the object in BeingCreated state
            // - Already has the object created...
            // We know value != NULL.  It could be kBeingCreatedMarker, or a valid ptr.
            // Unless your constructor can be very time consuming, it is very unlikely
            // to hit this race.  When it does, we just spin and yield the thread until
            // the object has been created.
            intptr_t value;
            while (true) {
                // The load has acquire memory ordering as the thread which reads the
                // instance pointer must acquire visibility over the associated data.
                // The pairing Release_Store operation is in Singleton::get().
                value = instance->load(std::memory_order_acquire);
                if (value != kBeingCreatedMarker)
                    break;
                PlatformThread::YieldCurrentThread();
            }
            return value;
        }

    }  // namespace internal
}  // namespace butil