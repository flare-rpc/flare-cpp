//
// Created by liyinbin on 2022/3/2.
//

#ifndef FLARE_STRINGS_UTILITY_H_
#define FLARE_STRINGS_UTILITY_H_
#include <string_view>
#include <string>
#include "flare/base/profile.h"

namespace flare {

    inline char front_char(const std::string_view &s) { return s[0]; }

    inline char back_char(const std::string_view &s) { return s[s.size() - 1]; }

    FLARE_MUST_USE_RESULT inline std::string as_string(const std::string_view &str) {
        return std::string(str.data(), str.size());
    }

    inline void copy_to_string(const std::string_view &str, std::string *out) {
        out->assign(str.data(), str.size());
    }

    inline void append_to_string(const std::string_view &str, std::string *out) {
        out->append(str.data(), str.size());
    }

}  // namespace flare

#endif // FLARE_STRINGS_UTILITY_H_
