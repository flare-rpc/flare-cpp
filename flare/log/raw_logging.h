//
// Created by liyinbin on 2022/2/13.
//

#ifndef FLARE_LOG_RAW_LOGGING_H_
#define FLARE_LOG_RAW_LOGGING_H_


#include <ctime>


#include "flare/log/severity.h"
#include "flare/log//vlog_is_on.h"

namespace flare::log {

// This is similar to LOG(severity) << format... and VLOG(level) << format..,
// but
// * it is to be used ONLY by low-level modules that can't use normal LOG()
// * it is desiged to be a low-level logger that does not allocate any
//   memory and does not need any locks, hence:
// * it logs straight and ONLY to STDERR w/o buffering
// * it uses an explicit format and arguments list
// * it will silently chop off really long message strings
// Usage example:
//   RAW_LOG(ERROR, "Failed foo with %i: %s", status, error);
//   RAW_VLOG(3, "status is %i", status);
// These will print an almost standard log lines like this to stderr only:
//   E20200821 211317 file.cc:123] RAW: Failed foo with 22: bad_file
//   I20200821 211317 file.cc:142] RAW: status is 20
#define RAW_LOG(severity, ...) \
  do { \
    switch (flare::log::FLARE_ ## severity) {  \
      case 0: \
        RAW_LOG_INFO(__VA_ARGS__); \
        break; \
      case 1: \
        RAW_LOG_WARNING(__VA_ARGS__); \
        break; \
      case 2: \
        RAW_LOG_ERROR(__VA_ARGS__); \
        break; \
      case 3: \
        RAW_LOG_FATAL(__VA_ARGS__); \
        break; \
      default: \
        break; \
    } \
  } while (0)

// The following STRIP_LOG testing is performed in the header file so that it's
// possible to completely compile out the logging code and the log messages.
#if STRIP_LOG == 0
#define RAW_VLOG(verboselevel, ...) \
  do { \
    if (VLOG_IS_ON(verboselevel)) { \
      RAW_LOG_TRACE(__VA_ARGS__); \
    } \
  } while (0)
#else
#define RAW_VLOG(verboselevel, ...) raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 0

#if STRIP_LOG == 0
#define RAW_LOG_TRACE(...) flare::log::RawLog__(flare::log::FLARE_TRACE, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_TRACE(...) flare::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 0

#if STRIP_LOG <= 1
#define RAW_LOG_DEBUG(...) flare::log::RawLog__(flare::log::FLARE_DEBUG, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_DEBUG(...) flare::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 1

#if STRIP_LOG <= 2
#define RAW_LOG_INFO(...) flare::log::RawLog__(flare::log::FLARE_INFO, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_INFO(...) flare::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG == 2

#if STRIP_LOG <= 3
#define RAW_LOG_WARNING(...) flare::log::RawLog__(flare::log::FLARE_WARNING,   \
                                      __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_WARNING(...) flare::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 3

#if STRIP_LOG <= 4
#define RAW_LOG_ERROR(...) flare::log::RawLog__(flare::log::FLARE_ERROR,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_ERROR(...) flare::log::raw_log_stub(0, __VA_ARGS__)
#endif // STRIP_LOG <= 4

#if STRIP_LOG <= 5
#define RAW_LOG_FATAL(...) flare::log::RawLog__(flare::log::FLARE_FATAL,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_FATAL(...) \
  do { \
    flare::log::raw_log_stub(0, __VA_ARGS__);        \
    exit(1); \
  } while (0)
#endif // STRIP_LOG <= 5

// Similar to CHECK(condition) << message,
// but for low-level modules: we use only RAW_LOG that does not allocate memory.
// We do not want to provide args list here to encourage this usage:
//   if (!cond)  RAW_LOG(FATAL, "foo ...", hard_to_compute_args);
// so that the args are not computed when not needed.
#define RAW_CHECK(condition, message)                                   \
  do {                                                                  \
    if (!(condition)) {                                                 \
      RAW_LOG(FATAL, "Check %s failed: %s", #condition, message);       \
    }                                                                   \
  } while (0)

// Debug versions of RAW_LOG and RAW_CHECK
#ifndef NDEBUG

#define RAW_DLOG(severity, ...) RAW_LOG(severity, __VA_ARGS__)
#define RAW_DCHECK(condition, message) RAW_CHECK(condition, message)

#else  // NDEBUG

#define RAW_DLOG(severity, ...)                                 \
  while (false)                                                 \
    RAW_LOG(severity, __VA_ARGS__)
#define RAW_DCHECK(condition, message) \
  while (false) \
    RAW_CHECK(condition, message)

#endif  // NDEBUG

    // Stub log function used to work around for unused variable warnings when
    // building with STRIP_LOG > 0.
    static inline void raw_log_stub(int /* ignored */, ...) {
    }

    // Helper function to implement RAW_LOG and RAW_VLOG
    // Logs format... at "severity" level, reporting it
    // as called from file:line.
    // This does not allocate memory or acquire locks.
    FLARE_EXPORT void RawLog__(flare::log::log_severity severity,
                               const char *file,
                               int line,
                               const char *format, ...);

}  // namespace flare::log

#endif // FLARE_LOG_RAW_LOGGING_H_

