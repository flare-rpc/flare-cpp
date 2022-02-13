//
// Created by jeff.li.
//

#include "flare/base/sysinfo.h"
#include "flare/bootstrap/bootstrap.h"
#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdio.h>

namespace flare::base {

    static void init_user_nane();

    static void init_host_name();

    std::string g_cache_username;
    std::string g_cache_host_name;

    void init_user_nane() {
        const char *user = getenv("USER");
        if (user != NULL) {
            g_cache_username = user;
        } else {
            struct passwd pwd;
            struct passwd *result = NULL;
            char buffer[1024] = {'\0'};
            uid_t uid = geteuid();
            int pwuid_res = getpwuid_r(uid, &pwd, buffer, sizeof(buffer), &result);
            if (pwuid_res == 0 && result) {
                g_cache_username = pwd.pw_name;
            } else {
                snprintf(buffer, sizeof(buffer), "uid%d", uid);
                g_cache_username = buffer;
            }
            if (g_cache_username.empty()) {
                g_cache_username = "invalid-user";
            }
        }

    }

    FLARE_BOOTSTRAP(0, [](){init_user_nane();});
    FLARE_BOOTSTRAP(0, [](){init_host_name();});

    const std::string &user_name() {
        return g_cache_username;
    }

    const std::string &get_host_name() {
        return g_cache_host_name;
    }

    void init_host_name() {
        struct utsname buf;
        if (0 != uname(&buf)) {
            // ensure null termination on failure
            *buf.nodename = '\0';
        }
        g_cache_host_name = buf.nodename;

    }


}  // namespace flare::base