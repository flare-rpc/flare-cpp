//
// Created by liyinbin on 2022/3/6.
//

#include "flare/files/readline_file.h"
#include "flare/files/sequential_read_file.h"
#include "flare/base/errno.h"
#include "flare/strings/str_split.h"
#include "flare/strings/utility.h"

namespace flare {

    flare::base::flare_status readline_file::open(const flare::file_path &path,  readline_option option) {
        FLARE_CHECK(_path.empty()) << "do not reopen";
        sequential_read_file file;
        _path = path;
        _status = file.open(_path);
        if (!_status.ok()) {
            FLARE_LOG(ERROR) << "open file :"<<_path<<" eroor "<< flare_error();
            return _status;
        }
        std::error_code ec;
        auto file_size = flare::file_size(path, ec);
        if (ec) {
            FLARE_LOG(ERROR) << "get file size :"<<_path<<" eroor "<< flare_error();
            _status.set_error(errno, "%s", flare_error());
        }
        _status = file.read(file_size, &_content);
        if (!_status.ok()) {
            FLARE_LOG(ERROR) << "read file :"<<_path<<" eroor "<< flare_error();
            return _status;
        }
        if(option == readline_option::eSkipEmptyLine) {
            _lines = flare::string_split(_content, flare::by_any_char("\n"), flare::skip_empty());
        } else if(option == readline_option::eTrimWhitespace) {
            _lines = flare::string_split(_content, flare::by_any_char("\n"), flare::skip_whitespace());
        } else {
            _lines = flare::string_split(_content, flare::by_any_char("\n"));
            _lines.pop_back();
        }
        for(size_t i = 0; i < _lines.size(); i++) {
            if(_lines[i].empty() && flare::back_char(_lines[i]) == '\r') {
                _lines[i].remove_suffix(1);
            }

        }
        return _status;
    }

}  // namespace flare