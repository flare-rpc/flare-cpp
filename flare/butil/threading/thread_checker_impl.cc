// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/butil/threading/thread_checker_impl.h"

namespace butil {

    ThreadCheckerImpl::ThreadCheckerImpl()
            : valid_thread_id_() {
        EnsureThreadIdAssigned();
    }

    ThreadCheckerImpl::~ThreadCheckerImpl() {}

    bool ThreadCheckerImpl::CalledOnValidThread() const {
        EnsureThreadIdAssigned();
        flare::base::AutoLock auto_lock(lock_);
        return valid_thread_id_ == PlatformThread::CurrentRef();
    }

    void ThreadCheckerImpl::DetachFromThread() {
        flare::base::AutoLock auto_lock(lock_);
        valid_thread_id_ = PlatformThreadRef();
    }

    void ThreadCheckerImpl::EnsureThreadIdAssigned() const {
        flare::base::AutoLock auto_lock(lock_);
        if (valid_thread_id_.is_null()) {
            valid_thread_id_ = PlatformThread::CurrentRef();
        }
    }

}  // namespace butil
