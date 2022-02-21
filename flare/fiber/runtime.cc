//
// Created by liyinbin on 2022/2/21.
//

#include "flare/fiber/internal/fiber.h"

namespace flare::fiber {

    int fiber_getconcurrency(void) {
        return flare::fiber_internal::fiber_getconcurrency();
    }

    int fiber_setconcurrency(int num) {
        return flare::fiber_internal::fiber_setconcurrency(num);
    }

}  // namespace flare::fiber
