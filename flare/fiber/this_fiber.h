//
// Created by liyinbin on 2022/2/20.
//

#ifndef FLARE_FIBER_THIS_FIBER_H_
#define FLARE_FIBER_THIS_FIBER_H_

#include <cstdint>

namespace flare {

    int fiber_yield();

    // expires_at_us should based microseconds flare::base::cpuwide_time_us();
    int fiber_sleep_until(const int64_t& expires_at_us);

    int fiber_sleep_for(const int64_t& expires_in_us);

    uint64_t get_fiber_id();

}  // namespace flare

#endif // FLARE_FIBER_THIS_FIBER_H_

