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

// Date: Tue Jul 10 17:40:58 CST 2012

#ifndef FLARE_FIBER_INTERNAL_SYS_FUTEX_H_
#define FLARE_FIBER_INTERNAL_SYS_FUTEX_H_

#include "flare/base/profile.h"         // FLARE_PLATFORM_OSX
#include <unistd.h>                     // syscall
#include <time.h>                       // timespec

#if defined(FLARE_PLATFORM_LINUX)
#include <syscall.h>                    // SYS_futex
#include <linux/futex.h>                // FUTEX_WAIT, FUTEX_WAKE

namespace flare::fiber_internal {

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif

inline int futex_wait_private(
    void* addr1, int expected, const timespec* timeout) {
    return syscall(SYS_futex, addr1, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG),
                   expected, timeout, NULL, 0);
}

inline int futex_wake_private(void* addr1, int nwake) {
    return syscall(SYS_futex, addr1, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG),
                   nwake, NULL, NULL, 0);
}

inline int futex_requeue_private(void* addr1, int nwake, void* addr2) {
    return syscall(SYS_futex, addr1, (FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG),
                   nwake, NULL, addr2, 0);
}

}  // namespace flare::fiber_internal

#elif defined(FLARE_PLATFORM_OSX)

namespace flare::fiber_internal {

    int futex_wait_private(void *addr1, int expected, const timespec *timeout);

    int futex_wake_private(void *addr1, int nwake);

    int futex_requeue_private(void *addr1, int nwake, void *addr2);

}  // namespace flare::fiber_internal

#else
#error "Unsupported OS"
#endif

#endif  // FLARE_FIBER_INTERNAL_SYS_FUTEX_H_
