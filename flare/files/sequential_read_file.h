//
// Created by liyinbin on 2022/3/6.
//

#ifndef FLARE_FILES_SEQUENTIAL_READ_FILE_H_
#define FLARE_FILES_SEQUENTIAL_READ_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/status.h"
#include "flare/io/cord_buf.h"

namespace flare {

    using flare::base::flare_status;

    class sequential_read_file {
    public:
        sequential_read_file() noexcept = default;

        ~sequential_read_file();

        flare_status open(const flare::file_path &path) noexcept;

        flare_status read(size_t n, std::string *content);

        flare_status read(size_t n, flare::cord_buf *buf);

        flare_status skip(size_t n);

        bool is_eof(flare_status *frs);

        size_t has_read() const {
            return _has_read;
        }

        const flare::file_path &path() const {
            return _path;
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(sequential_read_file);

        int _fd{-1};
        flare::file_path _path;
        size_t _has_read{0};
    };

}  // namespace flare

#endif  // FLARE_FILES_SEQUENTIAL_READ_FILE_H_
