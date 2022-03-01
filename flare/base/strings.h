//
// Created by liyinbin on 2022/1/22.
//

#ifndef FLARE_BASE_STRINGS_H_
#define FLARE_BASE_STRINGS_H_

#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <limits>
#include <stdarg.h>
#include <string_view>
#include "flare/base/profile.h"
#include "flare/base/ascii.h"

namespace flare::base {

    // Converts the characters in `s` to lowercase, changing the contents of `s`.
    void string_to_lower(std::string *s);

    // Creates a lowercase string from a given std::string_view.
    FLARE_MUST_USE_RESULT inline std::string string_to_lower(std::string_view s) {
        std::string result(s);
        flare::base::string_to_lower(&result);
        return result;
    }

    // Converts the characters in `s` to uppercase, changing the contents of `s`.
    void string_to_upper(std::string *s);

    // Creates an uppercase string from a given std::string_view.
    FLARE_MUST_USE_RESULT inline std::string string_to_upper(std::string_view s) {
        std::string result(s);
        flare::base::string_to_upper(&result);
        return result;
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
                                  memcmp(text.data() + (text.size() - suffix.size()), suffix.data(), suffix.size()) ==
                                  0);
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




    template<class T, class = void>
    struct try_parse_traits;

    template<class T, class... Args>
    inline std::optional<T> try_parse(const std::string_view &s,
                                      const Args &... args) {
        return try_parse_traits<T>::try_parse(s, args...);
    }


    template<class T, class>
    struct try_parse_traits {
        // The default implementation delegates calls to `TryParse` found by ADL.
        template<class... Args>
        static std::optional<T> try_parse(const std::string_view &s,
                                          const Args &... args) {
            return TryParse(std::common_type<T>(), s, args...);
        }
    };

    template<>
    struct try_parse_traits<bool> {
        // For numerical values, only 0 and 1 are recognized, all other numeric values
        // lead to parse failure (i.e., `std::nullopt` is returned).
        //
        // If `recognizes_alphabet_symbol` is set, following symbols are recognized:
        //
        // - "true" / "false"
        // - "y" / "n"
        // - "yes" / "no"
        //
        // For all other symbols, we treat them as neither `true` nor `false`.
        // `std::nullopt` is returned in those cases.
        static std::optional<bool> try_parse(const std::string_view &s,
                                             bool recognizes_alphabet_symbol = true,
                                             bool ignore_case = true);
    };

    template<class T>
    struct try_parse_traits<T, std::enable_if_t<std::is_integral_v<T>>> {
        static std::optional<T> try_parse(const std::string_view &s, int base = 10);
    };

    template<class T>
    struct try_parse_traits<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        static std::optional<T> try_parse(const std::string_view &s);
    };


}  // namespace flare::base
#endif  // FLARE_BASE_STRINGS_H_
