//
// Created by liyinbin on 2022/1/22.
//

#include "flare/base/strings.h"

namespace flare::base {
    namespace string_internal {
        int mem_ignore_case_cmp(const char* s1, const char* s2, size_t len);
    }

    void string_to_lower(std::string* s) {
        for (auto& ch : *s) {
            ch = flare::base::ascii_tolower(ch);
        }
    }

    void string_to_upper(std::string* s) {
        for (auto& ch : *s) {
            ch = flare::base::ascii_toupper(ch);
        }
    }


    void strip_extra_ascii_whitespace(std::string* str) {
        auto stripped = strip_ascii_whitespace(*str);

        if (stripped.empty()) {
            str->clear();
            return;
        }

        auto input_it = stripped.begin();
        auto input_end = stripped.end();
        auto output_it = &(*str)[0];
        bool is_ws = false;

        for (; input_it < input_end; ++input_it) {
            if (is_ws) {
                // Consecutive whitespace?  Keep only the last.
                is_ws = flare::base::ascii_isspace(*input_it);
                if (is_ws) --output_it;
            } else {
                is_ws = flare::base::ascii_isspace(*input_it);
            }

            *output_it = *input_it;
            ++output_it;
        }

        str->erase(output_it - &(*str)[0]);
    }

    namespace string_internal {
        int mem_ignore_case_cmp(const char* s1, const char* s2, size_t len) {
            const unsigned char* us1 = reinterpret_cast<const unsigned char*>(s1);
            const unsigned char* us2 = reinterpret_cast<const unsigned char*>(s2);

            for (size_t i = 0; i < len; i++) {
                const int diff =
                        int{static_cast<unsigned char>(flare::base::ascii_tolower(us1[i]))} -
                        int{static_cast<unsigned char>(flare::base::ascii_tolower(us2[i]))};
                if (diff != 0) return diff;
            }
            return 0;
        }
    }


    bool equals_ignore_case(std::string_view piece1,
                          std::string_view piece2) noexcept {
        return (piece1.size() == piece2.size() &&
            0 == string_internal::mem_ignore_case_cmp(piece1.data(), piece2.data(), piece1.size()));
    }

    bool starts_with_ignore_case(std::string_view text,
                              std::string_view prefix) noexcept {
        return (text.size() >= prefix.size()) && equals_ignore_case(text.substr(0, prefix.size()), prefix);
    }

    bool ends_with_ignore_case(std::string_view text,
                            std::string_view suffix) noexcept {
        return (text.size() >= suffix.size()) && equals_ignore_case(text.substr(text.size() - suffix.size()), suffix);
    }

}  // namespace flare::base