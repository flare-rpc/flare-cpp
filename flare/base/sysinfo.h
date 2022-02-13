//
// Created by liyinbin on 2022/2/13.
//

#ifndef FLARE_BASE_SYSINFO_H_
#define FLARE_BASE_SYSINFO_H_

#include <string>

namespace flare::base {

    const std::string &user_name();

    const std::string &get_host_name();

}  // namespace flare::base
#endif  // FLARE_BASE_SYSINFO_H_
