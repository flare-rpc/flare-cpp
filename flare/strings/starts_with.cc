// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#include "flare/strings/starts_with.h"
#include "flare/strings/compare.h"
#include <algorithm>

namespace flare::strings {

bool starts_with_case(std::string_view text, std::string_view suffix) {
    if (text.size() >= suffix.size()) {
        return flare::strings::compare_case(text.substr(0, suffix.size()), suffix) == 0;
    }
    return false;
}

}  // namespace flare::strings
