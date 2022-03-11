//
// Created by liyinbin on 2022/3/11.
//

#ifndef FLARE_FILES_SEQUENTIAL_WRITE_FILE_H_
#define FLARE_FILES_SEQUENTIAL_WRITE_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/status.h"
#include "flare/io/cord_buf.h"

namespace flare {
    using flare::base::flare_status;

    class sequential_write_file {
    public:
        sequential_write_file() = default;

        ~sequential_write_file();

        flare_status open(const flare::file_path &path, bool truncate = true) noexcept;

        flare_status write(std::string_view content);

        flare_status write(const flare::cord_buf &data);

        void flush();

        size_t has_write() const {
            return _has_write;
        }

        const flare::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        flare::file_path _path;
        size_t _has_write;
    };
}  // namespace flare
#endif  // FLARE_FILES_SEQUENTIAL_WRITE_FILE_H_
