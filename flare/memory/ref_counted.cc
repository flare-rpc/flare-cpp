// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flare/memory/ref_counted.h"

namespace flare::memory {

    namespace subtle {

        bool RefCountedThreadSafeBase::HasOneRef() const {
            return const_cast<RefCountedThreadSafeBase *>(this)->ref_count_.load(std::memory_order_acquire) == 1;
        }

        RefCountedThreadSafeBase::RefCountedThreadSafeBase() : ref_count_(0) {
#ifndef NDEBUG
            in_dtor_ = false;
#endif
        }

        RefCountedThreadSafeBase::~RefCountedThreadSafeBase() {
#ifndef NDEBUG
            DCHECK(in_dtor_) << "RefCountedThreadSafe object deleted without "
                                "calling Release()";
#endif
        }

        void RefCountedThreadSafeBase::AddRef() const {
#ifndef NDEBUG
            DCHECK(!in_dtor_);
#endif
            ref_count_.fetch_add(1, std::memory_order_relaxed);
        }

        bool RefCountedThreadSafeBase::Release() const {
#ifndef NDEBUG
            DCHECK(!in_dtor_);
            DCHECK(!ref_count_ != 0);
#endif
            if (ref_count_.fetch_add(-1, std::memory_order_relaxed) != 1) {
#ifndef NDEBUG
                in_dtor_ = true;
#endif
                return true;
            }
            return false;
        }

    }  // namespace subtle

}  // namespace flare::memory
