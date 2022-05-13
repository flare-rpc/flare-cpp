// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/base/scoped_file.h"
#include "flare/log/logging.h"

#if defined(FLARE_PLATFORM_POSIX)
#include <unistd.h>
#include "flare/base/profile/eintr_wrapper.h"
#endif

namespace flare::base {
    namespace internal {

#if defined(FLARE_PLATFORM_POSIX)

// static
        void ScopedFDCloseTraits::Free(int fd) {
            // It's important to crash here.
            // There are security implications to not closing a file descriptor
            // properly. As file descriptors are "capabilities", keeping them open
            // would make the current process keep access to a resource. Much of
            // Chrome relies on being able to "drop" such access.
            // It's especially problematic on Linux with the setuid sandbox, where
            // a single open directory would bypass the entire security model.
            FLARE_PCHECK(0 == IGNORE_EINTR(close(fd)));
        }

#endif  // FLARE_PLATFORM_POSIX

    }  // namespace internal
}  // namespace flare::base
