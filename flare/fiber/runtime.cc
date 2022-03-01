//
// Created by liyinbin on 2022/2/21.
//

#include "flare/fiber/internal/fiber.h"

namespace flare {

    int fiber_getconcurrency(void) {
        return ::fiber_getconcurrency();
    }

    int fiber_setconcurrency(int num) {
        return ::fiber_setconcurrency(num);
    }

}  // namespace flare
