//
// Created by liyinbin on 2022/1/22.
//

#include <sstream>
#include <stdexcept>
#include "flare/base/strings.h"
#include "flare/base/logging.h"

namespace flare::base {
    namespace string_internal {
        int mem_ignore_case_cmp(const char *s1, const char *s2, size_t len);
    }

    void string_to_lower(std::string *s) {
        for (auto &ch : *s) {
            ch = flare::base::ascii_tolower(ch);
        }
    }

    void string_to_upper(std::string *s) {
        for (auto &ch : *s) {
            ch = flare::base::ascii_toupper(ch);
        }
    }


    void strip_extra_ascii_whitespace(std::string *str) {
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
        int mem_ignore_case_cmp(const char *s1, const char *s2, size_t len) {
            const unsigned char *us1 = reinterpret_cast<const unsigned char *>(s1);
            const unsigned char *us2 = reinterpret_cast<const unsigned char *>(s2);

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


    std::string hex_dump(const void *const data, size_t size) {
        const unsigned char *const cdata =
                static_cast<const unsigned char *>(data);

        std::string out;
        out.resize(size * 2);

        static const char xdigits[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };

        std::string::iterator oi = out.begin();
        for (const unsigned char *si = cdata; si != cdata + size; ++si) {
            *oi++ = xdigits[(*si & 0xF0) >> 4];
            *oi++ = xdigits[(*si & 0x0F)];
        }

        return out;
    }

    std::string hex_dump(const std::string &str) {
        return hex_dump(str.data(), str.size());
    }

    std::string hex_dump(const std::vector<char> &data) {
        return hex_dump(data.data(), data.size());
    }

    std::string hex_dump(const std::vector<uint8_t> &data) {
        return hex_dump(data.data(), data.size());
    }

    std::string hex_dump_sourcecode(
            const std::string &str, const std::string &var_name) {

        std::ostringstream header;
        header << "const uint8_t " << var_name << "[" << str.size() << "] = {\n";

        static const int perline = 16;

        std::string out = header.str();
        out.reserve(out.size() + (str.size() * 5) - 1 + (str.size() / 16) + 4);

        static const char xdigits[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };

        std::string::size_type ci = 0;

        for (std::string::const_iterator si = str.begin(); si != str.end();
             ++si, ++ci) {

            out += "0x";
            out += xdigits[(*si & 0xF0) >> 4];
            out += xdigits[(*si & 0x0F)];

            if (ci + 1 < str.size()) {
                out += ',';

                if (ci % perline == perline - 1)
                    out += '\n';
            }
        }

        out += "\n};\n";

        return out;
    }

    /******************************************************************************/
    // Lowercase hex_dump Methods

    std::string hex_dump_lc(const void *const data, size_t size) {
        const unsigned char *const cdata =
                static_cast<const unsigned char *>(data);

        std::string out;
        out.resize(size * 2);

        static const char xdigits[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7',
                '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
        };

        std::string::iterator oi = out.begin();
        for (const unsigned char *si = cdata; si != cdata + size; ++si) {
            *oi++ = xdigits[(*si & 0xF0) >> 4];
            *oi++ = xdigits[(*si & 0x0F)];
        }

        return out;
    }

    std::string hex_dump_lc(const std::string &str) {
        return hex_dump_lc(str.data(), str.size());
    }

    std::string hex_dump_lc(const std::vector<char> &data) {
        return hex_dump_lc(data.data(), data.size());
    }

    std::string hex_dump_lc(const std::vector<uint8_t> &data) {
        return hex_dump_lc(data.data(), data.size());
    }

    /******************************************************************************/
    // Parser for Hex Digit Sequence

    std::string parse_hex_dump(const std::string &str) {
        std::string out;

        for (std::string::const_iterator si = str.begin(); si != str.end(); ++si) {

            unsigned char c = 0;

            // read first character of pair
            switch (*si) {
                case '0':
                    c |= 0x00;
                    break;
                case '1':
                    c |= 0x10;
                    break;
                case '2':
                    c |= 0x20;
                    break;
                case '3':
                    c |= 0x30;
                    break;
                case '4':
                    c |= 0x40;
                    break;
                case '5':
                    c |= 0x50;
                    break;
                case '6':
                    c |= 0x60;
                    break;
                case '7':
                    c |= 0x70;
                    break;
                case '8':
                    c |= 0x80;
                    break;
                case '9':
                    c |= 0x90;
                    break;
                case 'A':
                case 'a':
                    c |= 0xA0;
                    break;
                case 'B':
                case 'b':
                    c |= 0xB0;
                    break;
                case 'C':
                case 'c':
                    c |= 0xC0;
                    break;
                case 'D':
                case 'd':
                    c |= 0xD0;
                    break;
                case 'E':
                case 'e':
                    c |= 0xE0;
                    break;
                case 'F':
                case 'f':
                    c |= 0xF0;
                    break;
                default:
                    throw std::runtime_error("Invalid string for hex conversion");
            }

            ++si;
            if (si == str.end())
                throw std::runtime_error("Invalid string for hex conversion");

            // read second character of pair
            switch (*si) {
                case '0':
                    c |= 0x00;
                    break;
                case '1':
                    c |= 0x01;
                    break;
                case '2':
                    c |= 0x02;
                    break;
                case '3':
                    c |= 0x03;
                    break;
                case '4':
                    c |= 0x04;
                    break;
                case '5':
                    c |= 0x05;
                    break;
                case '6':
                    c |= 0x06;
                    break;
                case '7':
                    c |= 0x07;
                    break;
                case '8':
                    c |= 0x08;
                    break;
                case '9':
                    c |= 0x09;
                    break;
                case 'A':
                case 'a':
                    c |= 0x0A;
                    break;
                case 'B':
                case 'b':
                    c |= 0x0B;
                    break;
                case 'C':
                case 'c':
                    c |= 0x0C;
                    break;
                case 'D':
                case 'd':
                    c |= 0x0D;
                    break;
                case 'E':
                case 'e':
                    c |= 0x0E;
                    break;
                case 'F':
                case 'f':
                    c |= 0x0F;
                    break;
                default:
                    throw std::runtime_error("Invalid string for hex conversion");
            }

            out += static_cast<char>(c);
        }

        return out;
    }



// Copyright 2012 Facebook, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

    namespace {
        inline int string_printf_impl(std::string &output, const char *format,
                                      va_list args) {
            // Tru to the space at the end of output for our output buffer.
            // Find out write point then inflate its size temporarily to its
            // capacity; we will later shrink it to the size needed to represent
            // the formatted string.  If this buffer isn't large enough, we do a
            // resize and try again.

            const int write_point = output.size();
            int remaining = output.capacity() - write_point;
            output.resize(output.capacity());

            va_list copied_args;
            va_copy(copied_args, args);
            int bytes_used = vsnprintf(&output[write_point], remaining, format,
                                       copied_args);
            va_end(copied_args);
            if (bytes_used < 0) {
                return -1;
            } else if (bytes_used < remaining) {
                // There was enough room, just shrink and return.
                output.resize(write_point + bytes_used);
            } else {
                output.resize(write_point + bytes_used + 1);
                remaining = bytes_used + 1;
                bytes_used = vsnprintf(&output[write_point], remaining, format, args);
                if (bytes_used + 1 != remaining) {
                    return -1;
                }
                output.resize(write_point + bytes_used);
            }
            return 0;
        }
    }  // end anonymous namespace

    std::string string_printf(const char *format, ...) {
        // snprintf will tell us how large the output buffer should be, but
        // we then have to call it a second time, which is costly.  By
        // guestimating the final size, we avoid the double snprintf in many
        // cases, resulting in a performance win.  We use this constructor
        // of std::string to avoid a double allocation, though it does pad
        // the resulting string with nul bytes.  Our guestimation is twice
        // the format string size, or 32 bytes, whichever is larger.  This
        // is a hueristic that doesn't affect correctness but attempts to be
        // reasonably fast for the most common cases.
        std::string ret;
        ret.reserve(std::max(32UL, strlen(format) * 2));

        va_list ap;
        va_start(ap, format);
        if (string_printf_impl(ret, format, ap) != 0) {
            ret.clear();
        }
        va_end(ap);
        return ret;
    }

    // Basic declarations; allow for parameters of strings and string
    // pieces to be specified.
    int string_appendf(std::string *output, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        const size_t old_size = output->size();
        const int rc = string_printf_impl(*output, format, ap);
        if (rc != 0) {
            output->resize(old_size);
        }
        va_end(ap);
        return rc;
    }

    int string_vappendf(std::string *output, const char *format, va_list args) {
        const size_t old_size = output->size();
        const int rc = string_printf_impl(*output, format, args);
        if (rc != 0) {
            output->resize(old_size);
        }
        return rc;
    }

    int string_printf(std::string *output, const char *format, ...) {
        va_list ap;
        va_start(ap, format);
        output->clear();
        const int rc = string_printf_impl(*output, format, ap);
        if (rc != 0) {
            output->clear();
        }
        va_end(ap);
        return rc;
    }

    int string_vprintf(std::string *output, const char *format, va_list args) {
        output->clear();
        const int rc = string_printf_impl(*output, format, args);
        if (rc != 0) {
            output->clear();
        }
        return rc;
    }

    template<class T, class F, class... Args>
    std::optional<T> try_parse_impl(F fptr, const char *s,
                                    std::initializer_list<T> invs, Args... args) {
        if (FLARE_UNLIKELY(s[0] == 0)) {
            return std::nullopt;
        }
        char *end;
        auto result = fptr(s, &end, args...);
        if (end != s + strlen(s)) {  // Return value `0` is also handled by this test.
            return std::nullopt;
        }
        for (auto &&e : invs) {
            if (result == e && errno == ERANGE) {
                return std::nullopt;
            }
        }
        return result;
    }

    template<class T, class U>
    std::optional<T> try_narrow_cast(U &&opt) {
        if (!opt || *opt > std::numeric_limits<T>::max() ||
            *opt < std::numeric_limits<T>::min()) {
            return std::nullopt;
        }
        return opt;
    }

    std::optional<bool> try_parse_traits<bool>::try_parse(
            const std::string_view &s, bool recognizes_alphabet_symbol,
            bool ignore_case) {
        if (auto num_opt = flare::base::try_parse<int>(s); num_opt) {
            if (*num_opt == 0) {
                return false;
            } else if (*num_opt == 1) {
                return true;
            }
            return std::nullopt;
        }
        if (equals_ignore_case(s, "y") || equals_ignore_case(s, "yes") || equals_ignore_case(s, "true")) {
            return true;
        } else if (equals_ignore_case(s, "n") || equals_ignore_case(s, "no") || equals_ignore_case(s, "false")) {
            return false;
        }
        return std::nullopt;
    }

    template<class T>
    std::optional<T>
    try_parse_traits<T, std::enable_if_t<std::is_integral_v<T>>>::try_parse(
            const std::string_view &s, int base) {
        // `strtoll` expects a terminating null, therefore we copy `s` into this
        // temporary buffer before calling that method.
        //
        // Comparing to constructing a `std::string` here, this saves us a memory
        // allocation.
        char temp_buffer[129];
        if (FLARE_UNLIKELY(s.size() > 128)) {  // Out-of-range anyway.
            return {};
        }

        memcpy(temp_buffer, s.data(), s.size());
        temp_buffer[s.size()] = 0;

        // Here we always use the largest type to hold the result, and check if the
        // result is actually larger than what `T` can hold.
        //
        // It's not very efficient, though.
        if constexpr (std::is_signed_v<T>) {
            auto opt = try_parse_impl<std::int64_t>(&strtoll, temp_buffer,
                                                    {LLONG_MIN, LLONG_MAX}, base);
            return try_narrow_cast<T>(opt);
        } else {
            auto opt =
                    try_parse_impl<std::uint64_t>(&strtoull, temp_buffer, {ULLONG_MAX}, base);
            return try_narrow_cast<T>(opt);
        }
    }

    template<class T>
    std::optional<T>
    try_parse_traits<T, std::enable_if_t<std::is_floating_point_v<T>>>::try_parse(
            const std::string_view &s) {
        // For floating point types, there's no definitive limit on its length, so we
        // optimize for most common case and fallback to heap allocation for the rest.
        char temp_buffer[129];
        std::string heap_buffer;
        const char *ptr;
        if (FLARE_LIKELY(s.size() < 128)) {
            ptr = temp_buffer;
            memcpy(temp_buffer, s.data(), s.size());
            temp_buffer[s.size()] = 0;
        } else {
            heap_buffer.assign(s.data(), s.size());
            ptr = heap_buffer.data();
        }

        // We cannot use the same trick as `try_parse<integral>` here, as casting
        // between floating point is lossy.
        if (std::is_same_v<T, float>) {
            return try_parse_impl<float>(&strtof, ptr, {-HUGE_VALF, HUGE_VALF});
        } else if (std::is_same_v<T, double>) {
            return try_parse_impl<double>(&strtod, ptr, {-HUGE_VAL, HUGE_VAL});
        } else if (std::is_same_v<T, long double>) {
            return try_parse_impl<long double>(&strtold, ptr, {-HUGE_VALL, HUGE_VALL});
        }
        CHECK(0);
    }

#define INSTANTIATE_TRY_PARSE_TRAITS(type) template struct try_parse_traits<type>;

    INSTANTIATE_TRY_PARSE_TRAITS(char);                // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(signed char);         // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(signed short);        // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(signed int);          // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(signed long);         // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(signed long long);    // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(unsigned char);       // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(unsigned short);      // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(unsigned int);        // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(unsigned long);       // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(unsigned long long);  // NOLINT
    INSTANTIATE_TRY_PARSE_TRAITS(float);
    INSTANTIATE_TRY_PARSE_TRAITS(double);
    INSTANTIATE_TRY_PARSE_TRAITS(long double);

}  // namespace flare::base