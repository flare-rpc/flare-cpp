
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/base/sysinfo.h"
#include "flare/bootstrap/bootstrap.h"
#include <pwd.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
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

    FLARE_BOOTSTRAP(0, []() { init_user_nane(); });
    FLARE_BOOTSTRAP(0, []() { init_host_name(); });

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

    pid_t get_tid() {
        // On Linux and MacOSX, we try to use gettid().
#if defined FLARE_PLATFORM_LINUX || defined FLARE_PLATFORM_OSX
#ifndef __NR_gettid
#ifdef FLARE_PLATFORM_OSX
#define __NR_gettid SYS_gettid
#elif !defined __i386__
#error "Must define __NR_gettid for non-x86 platforms"
#else
#define __NR_gettid 224
#endif
#endif
        static bool lacks_gettid = false;
        if (!lacks_gettid) {
            pid_t tid = syscall(__NR_gettid);
            if (tid != -1) {
                return tid;
            }
            // Technically, this variable has to be volatile, but there is a small
            // performance penalty in accessing volatile variables and there should
            // not be any serious adverse effect if a thread does not immediately see
            // the value change to "true".
            lacks_gettid = true;
        }
#endif  // FLARE_PLATFORM_LINUX || FLARE_PLATFORM_OSX

        // If gettid() could not be used, we use one of the following.
#if defined FLARE_PLATFORM_LINUX
        return getpid();  // Linux:  getpid returns thread ID when gettid is absent
#else
        // If none of the techniques above worked, we use pthread_self().
        return (pid_t) (uintptr_t) pthread_self();
#endif
    }

    static int32_t g_main_thread_pid = ::getpid();

    int32_t get_main_thread_pid() {
        return g_main_thread_pid;
    }

    bool pid_has_changed() {
        int32_t pid = ::getpid();
        if (g_main_thread_pid == pid) {
            return false;
        }
        g_main_thread_pid = pid;
        return true;
    }

}  // namespace flare::base