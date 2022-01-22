//
// Created by liyinbin on 2022/1/22.
//

#include <sstream>
#include <stdexcept>
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


}  // namespace flare::base