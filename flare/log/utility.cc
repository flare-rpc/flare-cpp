//
// Created by liyinbin on 2022/2/13.
//
#include "flare/log/utility.h"
#include <syslog.h>
#include "flare/log/logging.h"
#include "flare/base/profile.h"
#include <sys/syscall.h>

#ifdef HAVE_STACKTRACE

#include "flare/debugging/stacktrace.h"
#include "flare/debugging/symbolize.h"
#include "flare/log/config.h"

FLARE_LOG_DEFINE_bool(symbolize_stacktrace, true,
                      "Symbolize the stack trace in the tombstone");

namespace flare::log {

    typedef void DebugWriter(const char *, void *);

    // The %p field width for printf() functions is two characters per byte.
    // For some environments, add two extra bytes for the leading "0x".
    static const int kPrintfPointerFieldWidth = 2 + 2 * sizeof(void *);

    static void DebugWriteToStderr(const char *data, void *) {
        // This one is signal-safe.
        if (write(STDERR_FILENO, data, strlen(data)) < 0) {
            // Ignore errors.
        }
    }

    static void DebugWriteToString(const char *data, void *arg) {
        reinterpret_cast<std::string *>(arg)->append(data);
    }


#ifdef HAVE_SYMBOLIZE

    // Print a program counter and its symbol name.
    static void DumpPCAndSymbol(DebugWriter *writerfn, void *arg, void *pc,
                                const char *const prefix) {
        char tmp[1024];
        const char *symbol = "(unknown)";
        // Symbolizes the previous address of pc because pc may be in the
        // next function.  The overrun happens when the function ends with
        // a call to a function annotated noreturn (e.g. CHECK).
        if (flare::debugging::symbolize(reinterpret_cast<char *>(pc) - 1, tmp, sizeof(tmp))) {
            symbol = tmp;
        }
        char buf[1024];
        snprintf(buf, sizeof(buf), "%s@ %*p  %s\n",
                 prefix, kPrintfPointerFieldWidth, pc, symbol);
        writerfn(buf, arg);
    }

#endif

    static void DumpPC(DebugWriter *writerfn, void *arg, void *pc,
                       const char *const prefix) {
        char buf[100];
        snprintf(buf, sizeof(buf), "%s@ %*p\n",
                 prefix, kPrintfPointerFieldWidth, pc);
        writerfn(buf, arg);
    }

// Dump current stack trace as directed by writerfn
    static void DumpStackTrace(int skip_count, DebugWriter *writerfn, void *arg) {
        // Print stack trace
        void *stack[32];
        int depth = flare::debugging::get_stack_trace(stack, FLARE_ARRAY_SIZE(stack), skip_count + 1);
        for (int i = 0; i < depth; i++) {
#if defined(HAVE_SYMBOLIZE)
            if (FLAGS_symbolize_stacktrace) {
                DumpPCAndSymbol(writerfn, arg, stack[i], "    ");
            } else {
                DumpPC(writerfn, arg, stack[i], "    ");
            }
#else
            DumpPC(writerfn, arg, stack[i], "    ");
#endif
        }
    }

    static void DumpStackTraceAndExit() {
        DumpStackTrace(1, DebugWriteToStderr, NULL);

        // TODO(hamaji): Use signal instead of sigaction?
        if (IsFailureSignalHandlerInstalled()) {
            // Set the default signal handler for SIGABRT, to avoid invoking our
            // own signal handler installed by InstallFailureSignalHandler().
#ifdef FLARE_PLATFORM_LINUX
            struct sigaction sig_action;
        memset(&sig_action, 0, sizeof(sig_action));
        sigemptyset(&sig_action.sa_mask);
        sig_action.sa_handler = SIG_DFL;
        sigaction(SIGABRT, &sig_action, NULL);
#endif  // HAVE_SIGACTION
        }

        abort();
    }

}  // namespace flare::log

#endif  // HAVE_STACKTRACE

namespace flare::log {

    namespace log_internal {
#ifdef HAVE_STACKTRACE

        void DumpStackTraceToString(std::string *stacktrace) {
            DumpStackTrace(1, DebugWriteToString, stacktrace);
        }

#endif
        static const char *g_program_invocation_short_name = NULL;

        const char *ProgramInvocationShortName() {
            if (g_program_invocation_short_name != NULL) {
                return g_program_invocation_short_name;
            } else {
                // TODO(hamaji): Use /proc/self/cmdline and so?
                return "UNKNOWN";
            }
        }

        bool IsGoogleLoggingInitialized() {
            return g_program_invocation_short_name != NULL;
        }

        void InitGoogleLoggingUtilities(const char *argv0) {
            CHECK(!IsGoogleLoggingInitialized())
                            << "You called InitGoogleLogging() twice!";
            const char *slash = strrchr(argv0, '/');
            g_program_invocation_short_name = slash ? slash + 1 : argv0;

#ifdef HAVE_STACKTRACE
            InstallFailureFunction(&DumpStackTraceAndExit);
#endif
        }

        void ShutdownGoogleLoggingUtilities() {
            CHECK(IsGoogleLoggingInitialized())
                            << "You called ShutdownGoogleLogging() without calling InitGoogleLogging() first!";
            g_program_invocation_short_name = NULL;
            closelog();
        }

        static int32_t g_main_thread_pid = ::getpid();

        int32_t GetMainThreadPid() {
            return g_main_thread_pid;
        }

        bool PidHasChanged() {
            int32_t pid = ::getpid();
            if (g_main_thread_pid == pid) {
                return false;
            }
            g_main_thread_pid = pid;
            return true;
        }


        pid_t GetTID() {
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

        static const CrashReason *g_reason = 0;

        void SetCrashReason(const CrashReason *r) {
            sync_val_compare_and_swap(&g_reason,
                                      reinterpret_cast<const CrashReason *>(0),
                                      r);
        }


        const char *const_basename(const char *filepath) {
            const char *base = strrchr(filepath, '/');
            return base ? (base + 1) : filepath;
        }


    }  // namespace log_internal
}  // namespace flare::log
