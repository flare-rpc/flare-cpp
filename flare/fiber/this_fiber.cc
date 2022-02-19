//
// Created by liyinbin on 2022/2/20.
//

#include "flare/fiber/this_fiber.h"
#include "flare/fiber/internal/task_group.h"
#include "flare/base/time.h"


namespace flare::fiber_internal {
    extern FLARE_THREAD_LOCAL TaskGroup *tls_task_group;
}  // namespace flare::fiber_internal

namespace flare::this_fiber {

    int fiber_yield() {
        flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            flare::fiber_internal::TaskGroup::yield(&g);
            return 0;
        }
        // pthread_yield is not available on MAC
        return sched_yield();
    }

    int fiber_sleep_until(const int64_t &expires_at_us) {
        int64_t now = flare::base::cpuwide_time_us();
        int64_t expires_in_us = expires_at_us - now;
        if (expires_in_us > 0) {
            return fiber_sleep_for(expires_in_us);
        } else {
            return fiber_yield();
        }

    }

    int fiber_sleep_for(const int64_t &expires_in_us) {
        flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
        if (nullptr != g && !g->is_current_pthread_task()) {
            return flare::fiber_internal::TaskGroup::usleep(&g, expires_in_us);
        }
        return ::usleep(expires_in_us);
    }
}  // namespace flare::this_fiber
