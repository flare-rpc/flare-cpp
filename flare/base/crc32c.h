
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_CRC32C_H_
#define FLARE_BASE_CRC32C_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <ostream>

namespace flare {

    class crc32c {
    public:
        crc32c() = default;

        crc32c(const crc32c &other) = default;

        crc32c(crc32c &&other) = default;

        crc32c &operator=(const crc32c &other) = default;

        crc32c &operator=(crc32c &&other) = default;

        explicit crc32c(uint32_t init_value) : _value(init_value) {}

        uint32_t extend(std::string_view sv) {
            return extend(sv.data(), sv.size());
        }

        uint32_t extend(const char *str, size_t n);

        uint32_t value() const {
            return _value;
        }

        // Return a masked representation of crc.
        //
        // Motivation: it is problematic to compute the CRC of a string that
        // contains embedded CRCs.  Therefore we recommend that CRCs stored
        // somewhere (e.g., in files) should be masked before being stored.
        static uint32_t mask(uint32_t crc) {
            // Rotate right by 15 bits and add a constant.
            return ((crc >> 15) | (crc << 17)) + kMaskDelta;
        }

        // Return the crc whose masked representation is masked_crc.
        static uint32_t unmask(uint32_t masked_crc) {
            uint32_t rot = masked_crc - kMaskDelta;
            return ((rot >> 17) | (rot << 15));
        }

        static bool is_fast_crc32_supported();

        crc32c &operator<<(std::string_view sv) {
            extend(sv);
            return *this;
        }

    private:
        static const uint32_t kMaskDelta = 0xa282ead8ul;
    private:
        uint32_t _value{0};
    };

    std::ostream &operator<<(std::ostream &os, const crc32c &crc) {
        os << crc.value();
        return os;
    }
}  // namespace flare

#endif  // FLARE_BASE_CRC32C_H_
