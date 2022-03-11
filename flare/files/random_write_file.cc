//
// Created by liyinbin on 2022/3/11.
//

#include "flare/files/random_write_file.h"
#include "flare/log/logging.h"
#include "flare/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace flare {

    random_write_file::~random_write_file() {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    flare_status random_write_file::open(const flare::file_path &path, bool truncate) noexcept {
        flare_status rs;
        if (truncate) {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
        } else {
            _fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0644);
        }
        if (_fd < 0) {
            LOG(ERROR) << "open file to append : " << path << "error: " << errno << " " << strerror(errno);
            rs.set_error(errno, "%s", strerror(errno));
        }
        return rs;
    }

    flare_status random_write_file::write(off_t offset, std::string_view content) {
        size_t size = content.size();
        off_t orig_offset = offset;
        flare_status frs;
        ssize_t left = size;
        const char* data = content.data();
        while (left > 0) {
            ssize_t written = ::pwrite(_fd, data, left, offset);
            if (written >= 0) {
                offset += written;
                left -= written;
                data += written;
            } else if (errno == EINTR) {
                continue;
            } else {
                LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                frs.set_error(errno, "%s", flare_error());
                return frs;
            }
        }

        return frs;
    }

    flare_status random_write_file::write(off_t offset, const flare::cord_buf &data) {
        size_t size = data.size();
        flare::cord_buf piece_data(data);
        off_t orig_offset = offset;
        flare_status frs;
        ssize_t left = size;
        while (left > 0) {
            ssize_t written = piece_data.pcut_into_file_descriptor(_fd, offset, left);
            if (written >= 0) {
                offset += written;
                left -= written;
            } else if (errno == EINTR) {
                continue;
            } else {
                LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                frs.set_error(errno, "%s", flare_error());
                return frs;
            }
        }

        return frs;
    }

    void flush();

}  // namespace flare
