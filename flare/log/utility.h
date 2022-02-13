//
// Created by liyinbin on 2022/2/13.
//

#ifndef FLARE_LOG_UTILITY_H_
#define FLARE_LOG_UTILITY_H_

#include <cstdint>
#include <string>

namespace flare::log {

    namespace log_internal {

        const char *ProgramInvocationShortName();

        bool IsGoogleLoggingInitialized();

        int32_t GetMainThreadPid();

        bool PidHasChanged();

        pid_t GetTID();

        // Get the part of filepath after the last path separator.
        // (Doesn't modify filepath, contrary to basename() in libgen.h.)
        const char *const_basename(const char *filepath);

        // Wrapper of __sync_val_compare_and_swap. If the GCC extension isn't
        // defined, we try the CPU specific logics (we only support x86 and
        // x86_64 for now) first, then use a naive implementation, which has a
        // race condition.
        template<typename T>
        inline T sync_val_compare_and_swap(T *ptr, T oldval, T newval) {
#if defined(HAVE___SYNC_VAL_COMPARE_AND_SWAP)
            return __sync_val_compare_and_swap(ptr, oldval, newval);
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
            T ret;
            __asm__ __volatile__("lock; cmpxchg %1, (%2);"
            :"=a"(ret)
            // GCC may produces %sil or %dil for
            // constraint "r", but some of apple's gas
            // dosn't know the 8 bit registers.
            // We use "q" to avoid these registers.
            :"q"(newval), "q"(ptr), "a"(oldval)
            :"memory", "cc");
            return ret;
#else
            T ret = *ptr;
  if (ret == oldval) {
    *ptr = newval;
  }
  return ret;
#endif
        }

        void DumpStackTraceToString(std::string *stacktrace);

        struct CrashReason {
            CrashReason() : filename(0), line_number(0), message(0), depth(0) {}

            const char *filename;
            int line_number;
            const char *message;

            // We'll also store a bit of stack trace context at the time of crash as
            // it may not be available later on.
            void *stack[32];
            int depth;
        };

        void SetCrashReason(const CrashReason *r);

        void InitGoogleLoggingUtilities(const char *argv0);

        void ShutdownGoogleLoggingUtilities();

    }  // namespace log_internal
}  // namespace flare::log

using namespace flare::log::log_internal;
#endif // FLARE_LOG_UTILITY_H_
