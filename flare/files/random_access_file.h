//
// Created by liyinbin on 2022/3/11.
//

#ifndef FLARE_FILES_RANDOM_ACCESS_FILE_H_
#define FLARE_FILES_RANDOM_ACCESS_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/status.h"
#include "flare/io/cord_buf.h"
#include <fcntl.h>

namespace flare {

    using flare::base::flare_status;

    class random_access_file {
    public:
        random_access_file() noexcept = default;

        ~random_access_file();

        flare_status open(const flare::file_path &path) noexcept;

        flare_status read(size_t n, off_t offset, std::string *content);

        flare_status read(size_t n, off_t offset, flare::cord_buf *buf);

        bool is_eof(off_t off, size_t has_read, flare_status *frs);


        const flare::file_path &path() const {
            return _path;
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(random_access_file);
        flare::file_path _path;
        int _fd{-1};
    };

}  // namespace flare

#endif // FLARE_FILES_RANDOM_ACCESS_FILE_H_
