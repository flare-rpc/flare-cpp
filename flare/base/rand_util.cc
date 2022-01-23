
#include "flare/base/rand_util.h"
#include <math.h>
#include <algorithm>
#include <limits>
#include "flare/base/logging.h"

namespace flare::base {

    template <class string_type>
    inline typename string_type::value_type* WriteInto(string_type* str,
                                                       size_t length_with_null) {
        DCHECK_GT(length_with_null, 1u);
        str->reserve(length_with_null);
        str->resize(length_with_null - 1);
        return &((*str)[0]);
    }

    int RandInt(int min, int max) {
        DCHECK_LE(min, max);

        uint64_t range = static_cast<uint64_t>(max) - min + 1;
        int result = min + static_cast<int>(flare::base::RandGenerator(range));
        DCHECK_GE(result, min);
        DCHECK_LE(result, max);
        return result;
    }

    double RandDouble() {
        return BitsToOpenEndedUnitInterval(flare::base::RandUint64());
    }

    double BitsToOpenEndedUnitInterval(uint64_t bits) {
        // We try to get maximum precision by masking out as many bits as will fit
        // in the target type's mantissa, and raising it to an appropriate power to
        // produce output in the range [0, 1).  For IEEE 754 doubles, the mantissa
        // is expected to accommodate 53 bits.

        static_assert(std::numeric_limits<double>::radix == 2, "otherwise use scalbn");
        static const int kBits = std::numeric_limits<double>::digits;
        uint64_t random_bits = bits & ((UINT64_C(1) << kBits) - 1);
        double result = ldexp(static_cast<double>(random_bits), -1 * kBits);
        DCHECK_GE(result, 0.0);
        DCHECK_LT(result, 1.0);
        return result;
    }

    uint64_t RandGenerator(uint64_t range) {
        DCHECK_GT(range, 0u);
        // We must discard random results above this number, as they would
        // make the random generator non-uniform (consider e.g. if
        // MAX_UINT64 was 7 and |range| was 5, then a result of 1 would be twice
        // as likely as a result of 3 or 4).
        uint64_t max_acceptable_value =
                (std::numeric_limits<uint64_t>::max() / range) * range - 1;

        uint64_t value;
        do {
            value = flare::base::RandUint64();
        } while (value > max_acceptable_value);

        return value % range;
    }

    std::string RandBytesAsString(size_t length) {
        DCHECK_GT(length, 0u);
        std::string result;
        RandBytes(WriteInto(&result, length + 1), length);
        return result;
    }

}  // namespace flare::base
