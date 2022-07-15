
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/base/result_status.h"
#include <cstring>
#include "flare/base/errno.h"

namespace flare {
    result_status result_status::from_flare_error(int err) {
        const char * msg = flare_error(err);
        return result_status(err, msg);
    }

    result_status result_status::from_flare_error(int err, const std::string_view &ext) {
        const char * msg = flare_error(err);
        return result_status(err, "{} {}", msg, ext);
    }

    result_status result_status::from_error_code(const std::error_code &ec) {
        return result_status(ec.value(), ec.message());
    }

}  // namespace flare
