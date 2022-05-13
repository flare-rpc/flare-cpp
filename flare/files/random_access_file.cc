//
// Created by liyinbin on 2022/3/11.
//

#include "flare/files/random_access_file.h"
#include "flare/log/logging.h"
#include "flare/base/errno.h"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace flare {


    random_access_file::~random_access_file() {
        if (_fd > 0) {
            ::close(_fd);
            _fd = -1;
        }
    }

    flare_status random_access_file::open(const flare::file_path &path) noexcept {
        FLARE_CHECK(_fd == -1)<<"do not reopen";
        flare_status rs;
        _path = path;
        _fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC, 0644);
        if (_fd < 0) {
            rs.set_error(errno, "%s", strerror(errno));
        }
        return rs;
    }

    flare_status random_access_file::read(size_t n, off_t offset, std::string *content) {
        flare::IOPortal portal;
        auto frs = read(n, offset, &portal);
        if (frs.ok()) {
            auto size = portal.size();
            portal.cutn(content, size);
        }
        return frs;
    }

    flare_status random_access_file::read(size_t n, off_t offset, flare::cord_buf *buf) {
        ssize_t left = n;
        flare::IOPortal portal;
        flare_status frs;
        off_t cur_off = offset;
        while (left > 0) {
            ssize_t read_len = portal.pappend_from_file_descriptor(_fd, cur_off, static_cast<size_t>(left));
            if (read_len > 0) {
                left -= read_len;
                cur_off += read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "read failed, errno: " <<errno<<" "<<flare_error()
                             << " fd: " << _fd << " size: " << n;
                frs.set_error(errno, "%s", flare_error());
                return frs;
            }
        }
        portal.swap(*buf);
        return frs;
    }

    bool random_access_file::is_eof(off_t off, size_t has_read, flare_status *frs) {
        std::error_code ec;
        auto size = flare::file_size(_path, ec);
        if (ec) {
            if (frs) {
                frs->set_error(errno, "%s", flare_error());
            }
            return false;
        }
        return off + has_read >= size;
    }

}  // namespace flare