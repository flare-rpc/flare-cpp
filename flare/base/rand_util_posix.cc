

#include "flare/base/rand_util.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "flare/butil/lazy_instance.h"
#include "flare/base/logging.h"

namespace {

// We keep the file descriptor for /dev/urandom around so we don't need to
// reopen it (which is expensive), and since we may not even be able to reopen
// it if we are later put in a sandbox. This class wraps the file descriptor so
// we can use LazyInstance to handle opening it on the first access.
    class URandomFd {
    public:
        URandomFd() : fd_(open("/dev/urandom", O_RDONLY)) {
            DCHECK_GE(fd_, 0) << "Cannot open /dev/urandom: " << errno;
        }

        ~URandomFd() { close(fd_); }

        int fd() const { return fd_; }

    private:
        const int fd_;
    };

    butil::LazyInstance<URandomFd>::Leaky g_urandom_fd = LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace flare::base {

    bool ReadFromFD(int fd, char* buffer, size_t bytes);
    // NOTE: This function must be cryptographically secure. http://crbug.com/140076
    uint64_t RandUint64() {
        uint64_t number;
        RandBytes(&number, sizeof(number));
        return number;
    }

    bool ReadFromFD(int fd, char* buffer, size_t bytes) {
        size_t total_read = 0;
        while (total_read < bytes) {
            ssize_t bytes_read =
                    HANDLE_EINTR(read(fd, buffer + total_read, bytes - total_read));
            if (bytes_read <= 0)
                break;
            total_read += bytes_read;
        }
        return total_read == bytes;
    }



    void RandBytes(void *output, size_t output_length) {
        const int urandom_fd = g_urandom_fd.Pointer()->fd();
        const bool success =
                ReadFromFD(urandom_fd, static_cast<char *>(output), output_length);
        CHECK(success);
    }

    int GetUrandomFD(void) {
        return g_urandom_fd.Pointer()->fd();
    }

}  // namespace flare::base
