//
// Created by liyinbin on 2022/1/22.
//

#ifndef FLARE_BASE_STRINGS_H_
#define FLARE_BASE_STRINGS_H_

#include <string>
#include <vector>
#include <cstring>
#include <stdarg.h>
#include <string_view>
#include "flare/base/profile.h"
#include "flare/base/ascii.h"

namespace flare::base {

    // Converts the characters in `s` to lowercase, changing the contents of `s`.
    void string_to_lower(std::string* s);

    // Creates a lowercase string from a given std::string_view.
    FLARE_MUST_USE_RESULT inline std::string string_to_lower(std::string_view s) {
        std::string result(s);
        flare::base::string_to_lower(&result);
        return result;
    }

    // Converts the characters in `s` to uppercase, changing the contents of `s`.
    void string_to_upper(std::string* s);

    // Creates an uppercase string from a given std::string_view.
    FLARE_MUST_USE_RESULT inline std::string string_to_upper(std::string_view s) {
        std::string result(s);
        flare::base::string_to_upper(&result);
        return result;
    }

    // Returns std::string_view with whitespace stripped from the beginning of the
    // given string_view.
    FLARE_MUST_USE_RESULT inline std::string_view strip_leading_ascii_whitespace(
            std::string_view str) {
        auto it = std::find_if_not(str.begin(), str.end(), flare::base::ascii_isspace);
        return str.substr(it - str.begin());
    }

    // Strips in place whitespace from the beginning of the given string.
    inline void strip_leading_ascii_whitespace(std::string* str) {
        auto it = std::find_if_not(str->begin(), str->end(), flare::base::ascii_isspace);
        str->erase(str->begin(), it);
    }

    // Returns std::string_view with whitespace stripped from the end of the given
    // string_view.
    FLARE_MUST_USE_RESULT inline std::string_view strip_trailing_ascii_whitespace(
            std::string_view str) {
        auto it = std::find_if_not(str.rbegin(), str.rend(), flare::base::ascii_isspace);
        return str.substr(0, str.rend() - it);
    }

    // Strips in place whitespace from the end of the given string
    inline void strip_trailing_ascii_whitespace(std::string* str) {
        auto it = std::find_if_not(str->rbegin(), str->rend(), flare::base::ascii_isspace);
        str->erase(str->rend() - it);
    }

    // Returns std::string_view with whitespace stripped from both ends of the
    // given string_view.
    FLARE_MUST_USE_RESULT inline std::string_view strip_ascii_whitespace(
            std::string_view str) {
        return strip_trailing_ascii_whitespace(strip_leading_ascii_whitespace(str));
    }

    // Strips in place whitespace from both ends of the given string
    inline void strip_ascii_whitespace(std::string* str) {
        strip_trailing_ascii_whitespace(str);
        strip_leading_ascii_whitespace(str);
    }

    // Strips leading, trailing, and consecutive internal whitespace.
    void strip_extra_ascii_whitespace(std::string*);

    FLARE_MUST_USE_RESULT inline std::string as_string(const std::string_view &str) {
        return std::string(str.data(), str.size());
    }

    inline void copy_to_string(const std::string_view &str, std::string *out) {
        out->assign(str.data(), str.size());
    }

    inline void append_to_string(const std::string_view &str, std::string *out) {
        out->append(str.data(), str.size());
    }

    // str_contains()
    //
    // Returns whether a given string `haystack` contains the substring `needle`.
    inline bool str_contains(std::string_view haystack,
                            std::string_view needle) noexcept {
        return haystack.find(needle, 0) != haystack.npos;
    }

    inline bool str_contains(std::string_view haystack, char needle) noexcept {
        return haystack.find(needle) != haystack.npos;
    }

    // starts_with()
    //
    // Returns whether a given string `text` begins with `prefix`.
    inline bool starts_with(std::string_view text,
                           std::string_view prefix) noexcept {
    return prefix.empty() || (text.size() >= prefix.size() &&
        memcmp(text.data(), prefix.data(), prefix.size()) == 0);
    }


    // ends_with()
    //
    // Returns whether a given string `text` ends with `suffix`.
    inline bool ends_with(std::string_view text,
                         std::string_view suffix) noexcept {
    return suffix.empty() || (text.size() >= suffix.size() &&
        memcmp(text.data() + (text.size() - suffix.size()), suffix.data(),suffix.size()) == 0);
    }

    // equals_ignore_case()
    //
    // Returns whether given ASCII strings `piece1` and `piece2` are equal, ignoring
    // case in the comparison.
    bool equals_ignore_case(std::string_view piece1,
                          std::string_view piece2) noexcept;

    // starts_with_ignore_case()
    //
    // Returns whether a given ASCII string `text` starts with `prefix`,
    // ignoring case in the comparison.
    bool starts_with_ignore_case(std::string_view text,
                              std::string_view prefix) noexcept;

    // ends_with_ignore_case()
    //
    // Returns whether a given ASCII string `text` ends with `suffix`, ignoring
    // case in the comparison.
    bool ends_with_ignore_case(std::string_view text,
                            std::string_view suffix) noexcept;


    // consume_prefix()
    //
    // Strips the `expected` prefix from the start of the given string, returning
    // `true` if the strip operation succeeded or false otherwise.
    //
    // Example:
    //
    //   std::string_view input("abc");
    //   EXPECT_TRUE(flare::base::consume_prefix(&input, "a"));
    //   EXPECT_EQ(input, "bc");
    inline bool consume_prefix(std::string_view* str, std::string_view expected) {
        if (!flare::base::starts_with(*str, expected)) {
            return false;
        }
        str->remove_prefix(expected.size());
        return true;
    }

    // consume_suffix()
    //
    // Strips the `expected` suffix from the end of the given string, returning
    // `true` if the strip operation succeeded or false otherwise.
    //
    // Example:
    //
    //   std::string_view input("abcdef");
    //   EXPECT_TRUE(flare::base::consume_suffix(&input, "def"));
    //   EXPECT_EQ(input, "abc");
    inline bool consume_suffix(std::string_view* str, std::string_view expected) {
        if (!flare::base::ends_with(*str, expected)) {
            return false;
        }
        str->remove_suffix(expected.size());
        return true;
    }

    // strip_prefix()
    //
    // Returns a view into the input string 'str' with the given 'prefix' removed,
    // but leaving the original string intact. If the prefix does not match at the
    // start of the string, returns the original string instead.
    FLARE_MUST_USE_RESULT inline std::string_view strip_prefix(std::string_view str, std::string_view prefix) {
        if (flare::base::starts_with(str, prefix)) {
            str.remove_prefix(prefix.size());
        }
        return str;
    }

    // strip_suffix()
    //
    // Returns a view into the input string 'str' with the given 'suffix' removed,
    // but leaving the original string intact. If the suffix does not match at the
    // end of the string, returns the original string instead.
    FLARE_MUST_USE_RESULT inline std::string_view strip_suffix(
            std::string_view str, std::string_view suffix) {
        if (flare::base::ends_with(str, suffix)) {
            str.remove_suffix(suffix.size());
        }
        return str;
    }

    // Uppercase Hex_dump Methods

    /*!
     * Dump a (binary) string as a sequence of uppercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \param size  length of binary data
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump(const void *const data, size_t size);

    /*!
     * Dump a (binary) string as a sequence of uppercase hexadecimal pairs.
     *
     * \param str  binary data to output in hex
     * \return     string of hexadecimal pairs
     */
    std::string hex_dump(const std::string &str);

    /*!
     * Dump a (binary) item as a sequence of uppercase hexadecimal pairs.
     *
     * \param t  binary data to output in hex
     * \return   string of hexadecimal pairs
     */
    template<typename Type>
    std::string hex_dump_type(const Type &t) {
        return hex_dump(&t, sizeof(t));
    }

    /*!
     * Dump a char vector as a sequence of uppercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump(const std::vector<char> &data);

    /*!
     * Dump a uint8_t vector as a sequence of uppercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump(const std::vector<uint8_t> &data);

    /*!
     * Dump a (binary) string into a C source code snippet. The snippet defines an
     * array of const uint8_t* holding the data of the string.
     *
     * \param str       string to output as C source array
     * \param var_name  name of the array variable in the outputted code snippet
     * \return          string holding C source snippet
     */
    std::string hex_dump_sourcecode(
            const std::string &str, const std::string &var_name = "name");

    /******************************************************************************/
    // Lowercase hex_dump Methods

    /*!
     * Dump a (binary) string as a sequence of lowercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \param size  length of binary data
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump_lc(const void *const data, size_t size);

    /*!
     * Dump a (binary) string as a sequence of lowercase hexadecimal pairs.
     *
     * \param str  binary data to output in hex
     * \return     string of hexadecimal pairs
     */
    std::string hex_dump_lc(const std::string &str);

    /*!
     * Dump a (binary) item as a sequence of lowercase hexadecimal pairs.
     *
     * \param t  binary data to output in hex
     * \return   string of hexadecimal pairs
     */
    template<typename Type>
    std::string hex_dump_lc_type(const Type &t) {
        return hex_dump_lc(&t, sizeof(t));
    }

    /*!
     * Dump a char vector as a sequence of lowercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump_lc(const std::vector<char> &data);

    /*!
     * Dump a uint8_t vector as a sequence of lowercase hexadecimal pairs.
     *
     * \param data  binary data to output in hex
     * \return      string of hexadecimal pairs
     */
    std::string hex_dump_lc(const std::vector<uint8_t> &data);

    /******************************************************************************/
    // Parser for Hex Digit Sequence

    /*!
     * Read a string as a sequence of hexadecimal pairs. Converts each pair of
     * hexadecimal digits into a byte of the output string. Throws
     * std::runtime_error() if an unknown letter is encountered.
     *
     * \param str  string to parse as hex digits
     * \return     string of read bytes
     */
    std::string parse_hex_dump(const std::string &str);


    // Convert |format| and associated arguments to std::string
    std::string string_printf(const char* format, ...)
    __attribute__ ((format (printf, 1, 2)));

    // Write |format| and associated arguments into |output|
    // Returns 0 on success, -1 otherwise.
    int string_printf(std::string* output, const char* fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Write |format| and associated arguments in form of va_list into |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vprintf(std::string* output, const char* format, va_list args);

    // Append |format| and associated arguments to |output|
    // Returns 0 on success, -1 otherwise.
    int string_appendf(std::string* output, const char* format, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Append |format| and associated arguments in form of va_list to |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vappendf(std::string* output, const char* format, va_list args);


}  // namespace flare::base
#endif  // FLARE_BASE_STRINGS_H_
