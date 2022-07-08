
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/metrics/bucket.h"
#include "flare/base/profile.h"
#include "flare/log/logging.h"

namespace flare {

    bucket bucket_builder::liner_values(int64_t start, int64_t width, size_t number) {
        FLARE_CHECK(width > 0);
        std::vector<int64_t> ret;
        ret.reserve(number);
        double value = start;
        for (size_t i = 0; i < number; ++i) {
            ret.push_back(value);
            value += width;
        }
        return ret;
    }

    bucket bucket_builder::exponential_values(int64_t start, int64_t factor, size_t number) {
        FLARE_CHECK(factor > 0);
        FLARE_CHECK(start > 0);
        std::vector<int64_t> ret;
        ret.reserve(number);
        double value = start;
        for (size_t i = 0; i < number; ++i) {
            ret.push_back(value);
            value *= factor;
        }
        return ret;
    }

    bucket bucket_builder::liner_duration(flare::duration start, flare::duration width, size_t num) {
        flare::duration value = start;
        FLARE_CHECK(width.to_int64_microseconds() > 0);
        std::vector<int64_t> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_int64_microseconds());
            value += width;
        }
        return ret;
    }

    bucket bucket_builder::exponential_duration(flare::duration start, uint64_t factor, size_t num) {
        flare::duration value = start;
        FLARE_CHECK(factor > 1);
        std::vector<int64_t> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_int64_microseconds());
            value *= factor;
        }
        return ret;
    }

}  // namespace abel
