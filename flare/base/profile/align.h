//
// Created by liyinbin on 2022/2/15.
//

#ifndef FLARE_ALIGN_H
#define FLARE_ALIGN_H

#include <cstddef>

namespace flare::base {
    constexpr std::size_t max_align_v = alignof(max_align_t);
}

#endif //FLARE_ALIGN_H
