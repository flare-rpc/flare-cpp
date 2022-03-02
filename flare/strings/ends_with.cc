// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "flare/strings/ends_with.h"
#include "flare/strings/compare.h"
#include "flare/strings/ascii.h"
#include <algorithm>
#include <cstring>

namespace flare {


    bool ends_with_ignore_case(std::string_view text, std::string_view suffix) noexcept{
        if (text.size() >= suffix.size())  {
            return flare::compare_case(text.substr(text.size() - suffix.size()), suffix) == 0;
        }
        return false;
    }

}  // namespace flare
