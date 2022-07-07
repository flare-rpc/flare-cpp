
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_SYSINFO_H_
#define FLARE_BASE_SYSINFO_H_

#include <string>
#include <cstdint>

namespace flare::base {

    const std::string &user_name();

    const std::string &get_host_name();

    pid_t get_tid();

    int32_t get_main_thread_pid();

    bool pid_has_changed();

}  // namespace flare::base
#endif  // FLARE_BASE_SYSINFO_H_
