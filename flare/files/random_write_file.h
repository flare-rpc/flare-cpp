//
// Created by liyinbin on 2022/3/11.
//

#ifndef FLARE_FILES_RANDOM_WRITE_FILE_H_
#define FLARE_FILES_RANDOM_WRITE_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/status.h"
#include "flare/io/cord_buf.h"
#include <fcntl.h>

namespace flare {

    using flare::base::flare_status;

    class random_write_file {
    public:
        random_write_file() = default;

        ~random_write_file();

        flare_status open(const flare::file_path &path, bool truncate = true) noexcept;

        flare_status write(off_t offset, std::string_view content);

        flare_status write(off_t offset, const flare::cord_buf &data);

        void flush();

        const flare::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        flare::file_path _path;
    };

}  // namespace flare

#endif  // FLARE_FILES_RANDOM_WRITE_FILE_H_
