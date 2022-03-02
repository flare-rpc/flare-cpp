// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef FLARE_STRINGS_COMPARE_H_
#define FLARE_STRINGS_COMPARE_H_

#include <string_view>
#include "flare/base/profile.h"

namespace flare {
/**
 * @brief returns +1/0/-1 like strcmp(a, b) but without regard for letter case
 * @param a todo
 * @param b todo
 * @return todo
 */
int compare_case(std::string_view a, std::string_view b);

FLARE_FORCE_INLINE bool equal_case(std::string_view a, std::string_view b) {
    return compare_case(a, b) == 0;
}

}  // namespace flare

#endif  // FLARE_STRINGS_COMPARE_H_
