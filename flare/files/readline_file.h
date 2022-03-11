//
// Created by liyinbin on 2022/3/6.
//

#ifndef FLARE_FILES_READLINE_FILE_H_
#define FLARE_FILES_READLINE_FILE_H_

#include <stdio.h>
#include <optional>
#include <string_view>
#include <string>
#include "flare/log/logging.h"
#include "flare/base/profile.h"
#include "flare/files/filesystem.h"
#include "flare/base/status.h"

namespace flare {
    // readline_file
    // read only
    // only hold by one object
    enum class readline_option {
        eNoSkip,
        eSkipEmptyLine,
        eTrimWhitespace
    };
    class readline_file {
    public:

        flare::base::flare_status open(const flare::file_path &path, readline_option option = readline_option::eSkipEmptyLine);

        size_t size() const noexcept {
            return _lines.size();
        }

        const file_path &path() const noexcept {
            return _path;
        }

        const std::vector<std::string_view> lines() const noexcept {
            return _lines;
        }

        operator bool() noexcept {
            return !_path.empty() && _status.ok();
        }

        flare::base::flare_status status() const noexcept{
            return _status;
        }

    private:
        std::string _content;
        file_path _path;
        flare::base::flare_status _status;
        std::vector<std::string_view> _lines;
    };
}  // namespace flare
#endif  // FLARE_FILES_READLINE_FILE_H_
