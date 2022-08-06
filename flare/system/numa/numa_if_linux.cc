
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/base/profile.h"

#ifdef FLARE_PLATFORM_LINUX

#include "flare/system/numa/numa_if.h"

namespace flare::system_internal {
    int32_t get_numa_max_possible_node() {
        return 1;
    }

    int32_t get_numa_max_node() {
        return 0;
    }
}  // namespace flare::system_internal
#endif  // FLARE_PLATFORM_OSX


