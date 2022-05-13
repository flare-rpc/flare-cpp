
#include <fcntl.h>                      // open
#include <stdio.h>                      // snprintf
#include <sys/types.h>  
#include <sys/uio.h>
#include <unistd.h>                     // read, gitpid
#include <sstream>                      // std::ostringstream
#include "flare/base/fd_guard.h"             // flare::base::fd_guard
#include "flare/log/logging.h"
#include "flare/base/popen.h"                // read_command_output
#include "flare/base/process_util.h"
#include "flare/base/profile.h"

namespace flare::base {

ssize_t read_command_line(char* buf, size_t len, bool with_args) {
#if defined(FLARE_PLATFORM_LINUX)
    flare::base::fd_guard fd(open("/proc/self/cmdline", O_RDONLY));
    if (fd < 0) {
        FLARE_LOG(ERROR) << "Fail to open /proc/self/cmdline";
        return -1;
    }
    ssize_t nr = read(fd, buf, len);
    if (nr <= 0) {
        FLARE_LOG(ERROR) << "Fail to read /proc/self/cmdline";
        return -1;
    }
#elif defined(FLARE_PLATFORM_OSX)
    static pid_t pid = getpid();
    std::ostringstream oss;
    char cmdbuf[32];
    snprintf(cmdbuf, sizeof(cmdbuf), "ps -p %ld -o command=", (long)pid);
    if (flare::base::read_command_output(oss, cmdbuf) != 0) {
        FLARE_LOG(ERROR) << "Fail to read cmdline";
        return -1;
    }
    const std::string& result = oss.str();
    ssize_t nr = std::min(result.size(), len);
    memcpy(buf, result.data(), nr);
#else
    #error Not Implemented
#endif

    if (with_args) {
        if ((size_t)nr == len) {
            return len;
        }
        for (ssize_t i = 0; i < nr; ++i) {
            if (buf[i] == '\0') {
                buf[i] = '\n';
            }
        }
        return nr;
    } else {
        for (ssize_t i = 0; i < nr; ++i) {
            // The command in macos is separated with space and ended with '\n'
            if (buf[i] == '\0' || buf[i] == '\n' || buf[i] == ' ') {
                return i;
            }
        }
        if ((size_t)nr == len) {
            FLARE_LOG(ERROR) << "buf is not big enough";
            return -1;
        }
        return nr;
    }
}

} // namespace flare::base
