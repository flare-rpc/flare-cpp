
#ifndef FLARE_BASE_LOGGING_H_
#define FLARE_BASE_LOGGING_H_

#include <inttypes.h>
#include <string>
#include <cstring>
#include <sstream>
#include "flare/base/profile.h"
#include "flare/base/static_atomic.h" // Used by LOG_EVERY_N, LOG_FIRST_N etc
#include "flare/base/time.h"      // gettimeofday_us()

# include <glog/logging.h>
# include <glog/raw_logging.h>
// define macros that not implemented in glog
# ifndef DCHECK_IS_ON   // glog didn't define DCHECK_IS_ON in older version
#  if defined(NDEBUG)
#    define DCHECK_IS_ON() 0
#  else
#    define DCHECK_IS_ON() 1
#  endif  // NDEBUG
# endif // DCHECK_IS_ON
# if DCHECK_IS_ON()
#  define DPLOG(...) PLOG(__VA_ARGS__)
#  define DPLOG_IF(...) PLOG_IF(__VA_ARGS__)
#  define DPCHECK(...) PCHECK(__VA_ARGS__)
#  define DVPLOG(...) VLOG(__VA_ARGS__)
# else
#  define DPLOG(...) DLOG(__VA_ARGS__)
#  define DPLOG_IF(...) DLOG_IF(__VA_ARGS__)
#  define DPCHECK(...) DCHECK(__VA_ARGS__)
#  define DVPLOG(...) DVLOG(__VA_ARGS__)
# endif

#define LOG_AT(severity, file, line)                                    \
    google::LogMessage(file, line, google::severity).stream()


#ifndef NOTIMPLEMENTED_POLICY
#if defined(FLARE_PLATFORM_ANDROID) && defined(OFFICIAL_BUILD)
#define NOTIMPLEMENTED_POLICY 0
#else
// Select default policy: LOG(ERROR)
#define NOTIMPLEMENTED_POLICY 4
#endif
#endif

#if defined(FLARE_COMPILER_GNUC) || defined(FLARE_COMPILER_CLANG)
// On Linux, with GCC, we can use __PRETTY_FUNCTION__ to get the demangled name
// of the current function in the NOTIMPLEMENTED message.
#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED() FLARE_EAT_STREAM_PARAMS
#elif NOTIMPLEMENTED_POLICY == 1
// TODO, figure out how to generate a warning
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED() do {                                   \
        static bool logged_once = false;                        \
        LOG_IF(ERROR, !logged_once) << NOTIMPLEMENTED_MSG;      \
        logged_once = true;                                     \
    } while(0);                                                 \
    FLARE_EAT_STREAM_PARAMS
#endif

#if defined(NDEBUG) && defined(OS_CHROMEOS)
#define NOTREACHED() LOG(ERROR) << "NOTREACHED() hit in "       \
    << __FUNCTION__ << ". "
#else
#define NOTREACHED() DCHECK(false)
#endif

// Helper macro included by all *_EVERY_N macros.
#define FLARE_LOG_IF_EVERY_N_IMPL(logifmacro, severity, condition, N)   \
    static std::atomic<int32_t> FLARE_CONCAT(logeveryn_, __LINE__){-1}; \
    const static int FLARE_CONCAT(logeveryn_sc_, __LINE__) = (N);       \
    const int FLARE_CONCAT(logeveryn_c_, __LINE__) =                    \
        FLARE_CONCAT(logeveryn_, __LINE__).fetch_add( 1) + 1; \
    logifmacro(severity, (condition) && FLARE_CONCAT(logeveryn_c_, __LINE__) / \
               FLARE_CONCAT(logeveryn_sc_, __LINE__) * FLARE_CONCAT(logeveryn_sc_, __LINE__) \
               == FLARE_CONCAT(logeveryn_c_, __LINE__))

// Helper macro included by all *_FIRST_N macros.
#define FLARE_LOG_IF_FIRST_N_IMPL(logifmacro, severity, condition, N)   \
    static std::atomic<int32_t> FLARE_CONCAT(logfstn_, __LINE__){0}; \
    logifmacro(severity, (condition) && FLARE_CONCAT(logfstn_, __LINE__) < N && \
               FLARE_CONCAT(logfstn_, __LINE__).fetch_add( 1) + 1 <= N)

// Helper macro included by all *_EVERY_SECOND macros.
#define FLARE_LOG_IF_EVERY_SECOND_IMPL(logifmacro, severity, condition) \
    static ::flare::base::EveryManyUS FLARE_CONCAT(logeverys_, __LINE__)(1000); \
    logifmacro(severity, (condition) && FLARE_CONCAT(logeverys_, __LINE__))

// ===============================================================

// Print a log for at most once. (not present in glog)
// Almost zero overhead when the log was printed.
#ifndef LOG_ONCE
# define LOG_ONCE(severity) LOG_FIRST_N(severity, 1)
# define LOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#endif

// Print a log after every N calls. First call always prints.
// Each call to this macro has a cost of relaxed atomic increment.
// The corresponding macro in glog is not thread-safe while this is.
#ifndef LOG_EVERY_N
# define LOG_EVERY_N(severity, N)                                \
     FLARE_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_EVERY_N(severity, condition, N)                  \
     FLARE_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print logs for first N calls.
// Almost zero overhead when the log was printed for N times
// The corresponding macro in glog is not thread-safe while this is.
#ifndef LOG_FIRST_N
# define LOG_FIRST_N(severity, N)                                \
     FLARE_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_FIRST_N(severity, condition, N)                  \
     FLARE_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print a log every second. (not present in glog). First call always prints.
// Each call to this macro has a cost of calling gettimeofday.
#ifndef LOG_EVERY_SECOND
# define LOG_EVERY_SECOND(severity)                                \
     FLARE_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, true)
# define LOG_IF_EVERY_SECOND(severity, condition)                \
     FLARE_LOG_IF_EVERY_SECOND_IMPL(LOG_IF, severity, condition)
#endif

#ifndef PLOG_EVERY_N
# define PLOG_EVERY_N(severity, N)                               \
     FLARE_LOG_IF_EVERY_N_IMPL(PLOG_IF, severity, true, N)
# define PLOG_IF_EVERY_N(severity, condition, N)                 \
     FLARE_LOG_IF_EVERY_N_IMPL(PLOG_IF, severity, condition, N)
#endif

#ifndef PLOG_FIRST_N
# define PLOG_FIRST_N(severity, N)                               \
     FLARE_LOG_IF_FIRST_N_IMPL(PLOG_IF, severity, true, N)
# define PLOG_IF_FIRST_N(severity, condition, N)                 \
     FLARE_LOG_IF_FIRST_N_IMPL(PLOG_IF, severity, condition, N)
#endif

#ifndef PLOG_ONCE
# define PLOG_ONCE(severity) PLOG_FIRST_N(severity, 1)
# define PLOG_IF_ONCE(severity, condition) PLOG_IF_FIRST_N(severity, condition, 1)
#endif

#ifndef PLOG_EVERY_SECOND
# define PLOG_EVERY_SECOND(severity)                             \
     FLARE_LOG_IF_EVERY_SECOND_IMPL(PLOG_IF, severity, true)
# define PLOG_IF_EVERY_SECOND(severity, condition)                       \
     FLARE_LOG_IF_EVERY_SECOND_IMPL(PLOG_IF, severity, condition)
#endif

// DEBUG_MODE is for uses like
//   if (DEBUG_MODE) foo.CheckThatFoo();
// instead of
//   #ifndef NDEBUG
//     foo.CheckThatFoo();
//   #endif
//
// We tie its state to ENABLE_DLOG.
enum {
    DEBUG_MODE = DCHECK_IS_ON()
};


#endif  // FLARE_BASE_LOGGING_H_
