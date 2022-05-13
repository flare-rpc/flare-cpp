//
// Created by liyinbin on 2022/2/6.
//
#include "flare/base/profile.h"

#if defined(FLARE_PLATFORM_POSIX)

#include <fcntl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "flare/io/fs_env.h"
#include "flare/log/logging.h"
#include "flare/base/errno.h"

namespace flare {

    // Common flags defined for all posix open operations
#if defined(FLARE_PLATFORM_LINUX)
    constexpr const int kOpenBaseFlags = O_CLOEXEC;
#else
    constexpr const int kOpenBaseFlags = 0;
#endif  // defined(FLARE_PLATFORM_LINUX)

    flare_status posix_file_error(const std::string &context, int error_number) {
        return flare_status(error_number, "file name: %s error: %s", context.c_str(), strerror(error_number));
    }

    using flare_status = flare::base::flare_status;

    class posix_random_access_file final : public random_access_file {
    public:
        explicit posix_random_access_file(const std::string &fname);

        flare::base::flare_status open() override;

        ~posix_random_access_file() override;

        flare_status read(uint64_t offset, size_t n, std::string *content) override;

        flare_status read(uint64_t offset, size_t n, flare::cord_buf *buf) override;

        void close() override;

    private:
        flare_status file_pread(uint64_t offset, size_t n, flare::IOPortal *portal);

    private:
        int _fd;
        std::string _file_name;
    };

    posix_random_access_file::posix_random_access_file(const std::string &fname)
            : _fd(-1),
              _file_name(fname) {

    }

    posix_random_access_file::~posix_random_access_file() {
        FLARE_CHECK(_fd == -1) << "fd leak, must close it";
    }

    flare::base::flare_status posix_random_access_file::open() {
        FLARE_CHECK(_fd == -1) << "do not open file once more";

        _fd = ::open(_file_name.c_str(), O_RDONLY | kOpenBaseFlags);
        if (FLARE_UNLIKELY(_fd < 0)) {
            return posix_file_error(_file_name, errno);
        }
        return flare::base::flare_status();
    }

    flare_status posix_random_access_file::file_pread(uint64_t offset, size_t n, flare::IOPortal *portal) {
        off_t orig_offset = offset;
        flare_status s;
        ssize_t left = n;
        while (left > 0) {
            ssize_t read_len = portal->pappend_from_file_descriptor(
                    _fd, offset, static_cast<size_t>(left));
            if (read_len > 0) {
                left -= read_len;
                offset += read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "read failed, err: " << flare_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << n;
                return posix_file_error(_file_name, errno);
            }
        }
        return flare_status();
    }


    flare_status posix_random_access_file::read(uint64_t offset, size_t n, std::string *content) {
        FLARE_CHECK(_fd != -1) << "must open file before reading";
        flare::IOPortal portal;
        auto s = file_pread(offset, n, &portal);
        if (s.ok()) {
            portal.append_to(content);
        }
        return s;
    }

    flare_status posix_random_access_file::read(uint64_t offset, size_t n, flare::cord_buf *buf) {
        FLARE_CHECK(_fd != -1) << "must open file before reading";
        flare::IOPortal portal;
        auto s = file_pread(offset, n, &portal);
        if (s.ok()) {
            portal.append_to(buf);
        }
        return s;
    }

    void posix_random_access_file::close() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }


    class posix_sequential_access_file final : public sequential_access_file {
    public:

        posix_sequential_access_file(const std::string &fname);

        ~posix_sequential_access_file();

        flare_status open() override;

        void close() override;

        flare_status read(size_t n, std::string *content) override;

        flare_status read(size_t n, flare::cord_buf *buf) override;

        flare_status skip(size_t n) override;

    private:
        flare_status file_read(size_t n, flare::IOPortal *portal);

    private:
        int _fd;
        std::string _file_name;
    };

    posix_sequential_access_file::posix_sequential_access_file(const std::string &fname)
            : _fd(-1),
              _file_name(fname) {
    }

    posix_sequential_access_file::~posix_sequential_access_file() {
        FLARE_CHECK(_fd == -1) << "must close it before dtor";
    }

    flare_status posix_sequential_access_file::open() {
        FLARE_CHECK(_fd == -1) << "do not open file once more";
        _fd = ::open(_file_name.c_str(), O_RDONLY | kOpenBaseFlags);
        if (FLARE_UNLIKELY(_fd < 0)) {
            return posix_file_error(_file_name, errno);
        }
        return flare::base::flare_status();
    }

    void posix_sequential_access_file::close() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    flare_status posix_sequential_access_file::file_read(size_t n, flare::IOPortal *portal) {
        flare_status s;
        ssize_t left = n;
        while (left > 0) {
            ssize_t read_len = portal->append_from_file_descriptor(_fd, static_cast<size_t>(left));
            if (read_len > 0) {
                left -= read_len;
            } else if (read_len == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "read failed, err: " << flare_error()
                             << " fd: " << _fd << " size: " << n;
                return posix_file_error(_file_name, errno);
            }
        }
        return s;
    }


    flare_status posix_sequential_access_file::read(size_t n, std::string *content) {

        flare::IOPortal portal;
        auto r = file_read(n, &portal);
        if (FLARE_LIKELY(r.ok())) {
            portal.append_to(content);
        }
        return r;
    }

    flare_status posix_sequential_access_file::read(size_t n, flare::cord_buf *buf) {
        flare::IOPortal portal;
        auto r = file_read(n, &portal);
        if (FLARE_LIKELY(r.ok())) {
            portal.append_to(buf);
        }
        return r;
    }

    flare_status posix_sequential_access_file::skip(size_t n) {
        if (::lseek(_fd, n, SEEK_CUR) == static_cast<off_t>(-1)) {
            return posix_file_error(_file_name, errno);
        }
        return flare_status();
    }

    class posix_writeable_file final : public writeable_file {
    public:
        posix_writeable_file(const std::string &fname);

        ~posix_writeable_file() override;

        flare_status open() override;

        void close() override;

        flare_status pos_write(uint64_t offset, flare::cord_buf *content) override;

        flare_status pos_write(uint64_t offset, const std::string_view &content) override;

        flare_status flush() override;

        flare_status sync() override;

    private:
        int _fd;
        std::string _file_name;
    };

    posix_writeable_file::posix_writeable_file(const std::string &fname)
            : _fd(-1),
              _file_name(fname) {

    }

    posix_writeable_file::~posix_writeable_file() {
        FLARE_CHECK(_fd == -1);
    }

    flare_status posix_writeable_file::open() {
        FLARE_CHECK(_fd == -1) << "do not open file once more";
        _fd = ::open(_file_name.c_str(), O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (FLARE_UNLIKELY(_fd < 0)) {
            return posix_file_error(_file_name, errno);
        }
        return flare::base::flare_status();
    }

    void posix_writeable_file::close() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    flare_status posix_writeable_file::pos_write(uint64_t offset, flare::cord_buf *content) {
        size_t size = content->size();
        flare::cord_buf piece_data(*content);
        off_t orig_offset = offset;
        ssize_t left = size;
        while (left > 0) {
            ssize_t written = piece_data.pcut_into_file_descriptor(_fd, offset, left);
            if (written >= 0) {
                offset += written;
                left -= written;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << size;
                return posix_file_error(_file_name, errno);
            }
        }

        return flare_status();
    }

    flare_status posix_writeable_file::pos_write(uint64_t offset, const std::string_view &content) {
        size_t left = content.size();
        off_t orig_offset = offset;
        const char *ptr = reinterpret_cast<const char *>(content.data());
        while (left > 0) {
            ssize_t nw = ::pwrite(_fd, ptr, left, offset);
            if (nw > 0) {
                left -= nw;
                offset += nw;
                ptr += static_cast<ptrdiff_t>(nw);
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " offset: " << orig_offset << " size: " << content.size();
                return posix_file_error(_file_name, errno);
            }
        }
        return flare_status();
    }

    flare_status posix_writeable_file::flush() {
        return flare_status();
    }

    flare_status posix_writeable_file::sync() {
        return flare_status();
    }

    class posix_append_file : public append_file {
    public:
        posix_append_file(const std::string &fname);

        ~posix_append_file() override;

        flare_status open() override;

        void close() override;

        flare_status append(flare::cord_buf *content) override;

        flare_status append(const std::string_view &content) override;

        flare_status flush() override;

        flare_status sync() override;

    private:
        int _fd;
        std::string _file_name;
    };

    posix_append_file::posix_append_file(const std::string &fname) : _fd(-1), _file_name(fname) {}


    posix_append_file::~posix_append_file() {
        FLARE_CHECK(_fd == -1);
    }

    flare_status posix_append_file::open() {
        FLARE_CHECK(_fd == -1) << "do not open file once more";
        _fd = ::open(_file_name.c_str(), O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (FLARE_UNLIKELY(_fd < 0)) {
            return posix_file_error(_file_name, errno);
        }
        return flare::base::flare_status();
    }

    void posix_append_file::close() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    flare_status posix_append_file::append(flare::cord_buf *content) {
        size_t size = content->size();
        flare::cord_buf piece_data(*content);
        ssize_t left = size;
        while (left > 0) {
            ssize_t written = piece_data.cut_into_file_descriptor(_fd, left);
            if (written >= 0) {
                left -= written;
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " size: " << size;
                return posix_file_error(_file_name, errno);
            }
        }

        return flare_status();
    }

    flare_status posix_append_file::append(const std::string_view &content) {
        size_t left = content.size();
        const char *ptr = reinterpret_cast<const char *>(content.data());
        while (left > 0) {
            ssize_t nw = ::write(_fd, ptr, left);
            if (nw > 0) {
                left -= nw;
                ptr += static_cast<ptrdiff_t>(nw);
            } else if (errno == EINTR) {
                continue;
            } else {
                FLARE_LOG(WARNING) << "write falied, err: " << flare_error()
                             << " fd: " << _fd << " size: " << content.size();
                return posix_file_error(_file_name, errno);
            }
        }
        return flare_status();
    }

    flare_status posix_append_file::flush() {
        return flare_status();
    }

    flare_status posix_append_file::sync() {
        return flare_status();
    }

    class posix_fs_env {
    public:
        flare_status
        get_random_access_file(const std::string &path, auto_close_ptr<random_access_file> &file) {
            file.reset(new posix_random_access_file(path));
            return flare_status();
        }

        flare_status
        get_sequential_access_file(const std::string &path, auto_close_ptr<sequential_access_file> &file) {
            file.reset(new posix_sequential_access_file(path));
            return flare_status();
        }

        flare_status
        get_writeable_file(const std::string &path, auto_close_ptr<writeable_file> &file) {
            file.reset(new posix_writeable_file(path));
            return flare_status();
        }

        flare_status
        get_append_file(const std::string &path, auto_close_ptr<append_file> &file) {
            file.reset(new posix_append_file(path));
            return flare_status();
        }
    };

    static posix_fs_env g_fs_env;
    fs_env *fs_ptr = reinterpret_cast<fs_env *>(&g_fs_env);

    fs_env *fs_env::default_disk_env() {
        return fs_ptr;
    }
}  // namespace flare

#endif  // defined(FLARE_PLATFORM_POSIX)
