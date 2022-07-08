
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <sys/stat.h>
#include "flare/base/profile.h"              // FLARE_PLATFORM_OSX
#include "flare/files/file_watcher.h"

namespace flare {

    static const file_watcher::Timestamp NON_EXIST_TS =
            static_cast<file_watcher::Timestamp>(-1);

    file_watcher::file_watcher() : _last_ts(NON_EXIST_TS) {
    }

    int file_watcher::init(const char *file_path) {
        if (init_from_not_exist(file_path) != 0) {
            return -1;
        }
        check_and_consume(NULL);
        return 0;
    }

    int file_watcher::init_from_not_exist(const char *file_path) {
        if (NULL == file_path) {
            return -1;
        }
        if (!_file_path.empty()) {
            return -1;
        }
        _file_path = file_path;
        return 0;
    }

    file_watcher::Change file_watcher::check(Timestamp *new_timestamp) const {
        struct stat st;
        const int ret = stat(_file_path.c_str(), &st);
        if (ret < 0) {
            *new_timestamp = NON_EXIST_TS;
            if (NON_EXIST_TS != _last_ts) {
                return DELETED;
            } else {
                return UNCHANGED;
            }
        } else {
            // Use microsecond timestamps which can be used for:
            //   2^63 / 1000000 / 3600 / 24 / 365 = 292471 years
            const Timestamp cur_ts =
#if defined(FLARE_PLATFORM_OSX)
                    st.st_mtimespec.tv_sec * 1000000L + st.st_mtimespec.tv_nsec / 1000L;
#else
            st.st_mtim.tv_sec * 1000000L + st.st_mtim.tv_nsec / 1000L;
#endif
            *new_timestamp = cur_ts;
            if (NON_EXIST_TS != _last_ts) {
                if (cur_ts != _last_ts) {
                    return UPDATED;
                } else {
                    return UNCHANGED;
                }
            } else {
                return CREATED;
            }
        }
    }

    file_watcher::Change file_watcher::check_and_consume(Timestamp *last_timestamp) {
        Timestamp new_timestamp;
        Change e = check(&new_timestamp);
        if (last_timestamp) {
            *last_timestamp = _last_ts;
        }
        if (e != UNCHANGED) {
            _last_ts = new_timestamp;
        }
        return e;
    }

    void file_watcher::restore(Timestamp timestamp) {
        _last_ts = timestamp;
    }

}  // namespace flare
