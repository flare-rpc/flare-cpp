// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLARE_BASE_RAND_UTIL_H_
#define FLARE_BASE_RAND_UTIL_H_

#include <string>

#include "flare/base/profile.h"

namespace flare::base {

    // --------------------------------------------------------------------------
    // NOTE(gejun): Functions in this header read from /dev/urandom in posix
    // systems and are not proper for performance critical situations.
    // For fast random numbers, check fast_rand.h
    // --------------------------------------------------------------------------

    // Returns a random number in range [0, kuint64max]. Thread-safe.
    FLARE_EXPORT uint64_t RandUint64();

    // Returns a random number between min and max (inclusive). Thread-safe.
    FLARE_EXPORT int RandInt(int min, int max);

    // Returns a random number in range [0, range).  Thread-safe.
    //
    // Note that this can be used as an adapter for std::random_shuffle():
    // Given a pre-populated |std::vector<int> myvector|, shuffle it as
    //   std::random_shuffle(myvector.begin(), myvector.end(), butil::RandGenerator);
    FLARE_EXPORT uint64_t RandGenerator(uint64_t range);

    // Returns a random double in range [0, 1). Thread-safe.
    FLARE_EXPORT double RandDouble();

    // Given input |bits|, convert with maximum precision to a double in
    // the range [0, 1). Thread-safe.
    FLARE_EXPORT double BitsToOpenEndedUnitInterval(uint64_t bits);

    // Fills |output_length| bytes of |output| with random data.
    //
    // WARNING:
    // Do not use for security-sensitive purposes.
    // See crypto/ for cryptographically secure random number generation APIs.
    FLARE_EXPORT void RandBytes(void *output, size_t output_length);

    // Fills a string of length |length| with random data and returns it.
    // |length| should be nonzero.
    //
    // Note that this is a variation of |RandBytes| with a different return type.
    // The returned string is likely not ASCII/UTF-8. Use with care.
    //
    // WARNING:
    // Do not use for security-sensitive purposes.
    // See crypto/ for cryptographically secure random number generation APIs.
    FLARE_EXPORT std::string RandBytesAsString(size_t length);

#if defined(FLARE_PLATFORM_POSIX)
    FLARE_EXPORT int GetUrandomFD();

#endif

}  // namespace flare::base

#endif  // FLARE_BASE_RAND_UTIL_H_
