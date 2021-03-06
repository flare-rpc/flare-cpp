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


#ifndef  FLARE_RPC_USERCODE_BACKUP_POOL_H_
#define  FLARE_RPC_USERCODE_BACKUP_POOL_H_

#include "flare/base/static_atomic.h"
#include "flare/fiber/internal/fiber.h"
#include <gflags/gflags_declare.h>


namespace flare::rpc {

    DECLARE_bool(usercode_in_pthread);
    DECLARE_int32(usercode_backup_threads);

    // "user code backup pool" is a set of pthreads to run user code when #pthread
    // workers of fibers reaches a threshold, avoiding potential deadlock when
    // -usercode_in_pthread is on. These threads are NOT supposed to be active
    // frequently, if they're, user should configure more num_threads for the
    // server or set -fiber_concurrency to a larger value.

    // Run the user code in-place or in backup threads. The place depends on
    // busy-ness of fiber workers.
    // NOTE: To avoid memory allocation(for `arg') in the case of in-place running,
    // check out the inline impl. of this function just below.
    void RunUserCode(void (*fn)(void *), void *arg);

    // RPC code should check this function before submitting operations that have
    // user code to run laterly.
    inline bool TooManyUserCode() {
        extern bool g_too_many_usercode;
        return g_too_many_usercode;
    }

    // If this function returns true, the user code is suggested to be run in-place
    // and user should call EndRunningUserCodeInPlace() after running the code.
    // Otherwise, user should call EndRunningUserCodeInPool() to run the user code
    // in backup threads.
    // Check RunUserCode() below to see the usage pattern.
    inline bool BeginRunningUserCode() {
        extern flare::static_atomic<int> g_usercode_inplace;
        return (g_usercode_inplace.fetch_add(1, std::memory_order_relaxed)
                + FLAGS_usercode_backup_threads) < fiber_getconcurrency();
    }

    inline void EndRunningUserCodeInPlace() {
        extern flare::static_atomic<int> g_usercode_inplace;
        g_usercode_inplace.fetch_sub(1, std::memory_order_relaxed);
    }

    void EndRunningUserCodeInPool(void (*fn)(void *), void *arg);

    // Incorporate functions above together. However `arg' to this function often
    // has to be new-ed even for in-place cases. If performance is critical, use
    // the BeginXXX/EndXXX pattern.
    inline void RunUserCode(void (*fn)(void *), void *arg) {
        if (BeginRunningUserCode()) {
            fn(arg);
            EndRunningUserCodeInPlace();
        } else {
            EndRunningUserCodeInPool(fn, arg);
        }
    }

    // [Optional] initialize the pool of backup threads. If this function is not
    // called, it will be called in EndRunningUserCodeInPool
    void InitUserCodeBackupPoolOnceOrDie();

} // namespace flare::rpc


#endif  // FLARE_RPC_USERCODE_BACKUP_POOL_H_
