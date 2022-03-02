//
// Created by liyinbin on 2022/1/30.
//

#ifndef FLARE_STRINGS_SAFE_SUBSTR_H_
#define FLARE_STRINGS_SAFE_SUBSTR_H_

namespace flare {

    inline std::string_view safe_substr(const std::string_view &sv, size_t pos = 0, size_t n = std::string_view::npos) noexcept {
        if (pos > sv.size()) {
            pos = sv.size();
        }
        if (n > (sv.size() - pos)) {
            n = sv.size() - pos;
        }
        return std::string_view(sv.data() + pos, n);
    }

}  // namespace flare

#endif // FLARE_STRINGS_SAFE_SUBSTR_H_

