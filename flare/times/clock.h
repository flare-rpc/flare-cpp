// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef FLARE_TIMES_CLOCK_H_
#define FLARE_TIMES_CLOCK_H_

#include "flare/base/profile.h"
#include "flare/times/time.h"

namespace flare {


// now()
//
// Returns the current time, expressed as an `flare::time_point` absolute time value.
    flare::time_point time_now();

// get_current_time_nanos()
//
// Returns the current time, expressed as a count of nanoseconds since the Unix
// Epoch (https://en.wikipedia.org/wiki/Unix_time). Prefer `flare::time_now()` instead
// for all but the most performance-sensitive cases (i.e. when you are calling
// this function hundreds of thousands of times per second).
    int64_t get_current_time_nanos();

// sleep_for()
//
// Sleeps for the specified duration, expressed as an `flare::duration`.
//
// Notes:
// * signal interruptions will not reduce the sleep duration.
// * Returns immediately when passed a nonpositive duration.
    void sleep_for(flare::duration duration);


}  // namespace flare

// -----------------------------------------------------------------------------
// Implementation Details
// -----------------------------------------------------------------------------

// In some build configurations we pass --detect-odr-violations to the
// gold linker.  This causes it to flag weak symbol overrides as ODR
// violations.  Because ODR only applies to C++ and not C,
// --detect-odr-violations ignores symbols not mangled with C++ names.
// By changing our extension points to be extern "C", we dodge this
// check.
extern "C" {
void flare_internal_sleep_for(flare::duration duration);
}  // extern "C"

FLARE_FORCE_INLINE void flare::sleep_for(flare::duration duration) {
    flare_internal_sleep_for(duration);
}

#endif  // FLARE_TIMES_CLOCK_H_
