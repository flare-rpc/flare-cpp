//
// Created by jeff.li.
//

#ifndef FLARE_LOG_LOGGING_H_
#define FLARE_LOG_LOGGING_H_


#include <cerrno>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iosfwd>
#include <ostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <cstdint>
#include <atomic>

#include <gflags/gflags.h>
#include "flare/base/profile.h"
#include "flare/strings/fmt/format.h"
#include "flare/strings/fmt/ostream.h"
#include "flare/log/config.h"
#include "flare/log/severity.h"
#include "flare/log/vlog_is_on.h"
#include "flare/log/utility.h"
#include "flare/base/profile.h"
#include "flare/base/static_atomic.h" // Used by LOG_EVERY_N, LOG_FIRST_N etc
#include "flare/times/time.h"

// The global value of FLARE_STRIP_LOG. All the messages logged to
// LOG(XXX) with severity less than FLARE_STRIP_LOG will not be displayed.
// If it can be determined at compile time that the message will not be
// printed, the statement will be compiled out.
//
// Example: to strip out all INFO and WARNING messages, use the value
// of 2 below. To make an exception for WARNING messages from a single
// file, add "#define FLARE_STRIP_LOG 1" to that file _before_ including
// base/logging.h
#ifndef FLARE_STRIP_LOG
#define FLARE_STRIP_LOG 0
#endif


// Log messages below the FLARE_STRIP_LOG level will be compiled away for
// security reasons. See LOG(severtiy) below.

// A few definitions of macros that don't generate much code.  Since
// LOG(INFO) and its ilk are used all over our code, it's
// better to have compact code for these operations.

#if FLARE_STRIP_LOG == 0
#define COMPACT_FLARE_LOG_TRACE flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_TRACE)
#define LOG_TO_STRING_TRACE(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_TRACE, message)
#else
#define COMPACT_FLARE_LOG_TRACE flare::log::null_stream()
#define LOG_TO_STRING_TRACE(message) flare::log::null_stream()
#endif

#if FLARE_STRIP_LOG <= 1
#define COMPACT_FLARE_LOG_DEBUG flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_DEBUG)
#define LOG_TO_STRING_DEBUG(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_DEBUG, message)
#else
#define COMPACT_FLARE_LOG_DEBUG flare::log::null_stream()
#define LOG_TO_STRING_DEBUG(message) flare::log::null_stream()
#endif

#if FLARE_STRIP_LOG <= 2
#define COMPACT_FLARE_LOG_INFO flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_INFO)
#define LOG_TO_STRING_INFO(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_INFO, message)
#else
#define COMPACT_FLARE_LOG_INFO flare::log::null_stream()
#define LOG_TO_STRING_INFO(message) flare::log::null_stream()
#endif

#if FLARE_STRIP_LOG <= 3
#define COMPACT_FLARE_LOG_WARNING flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_WARNING)
#define LOG_TO_STRING_WARNING(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_WARNING, message)
#else
#define COMPACT_FLARE_LOG_WARNING flare::log::null_stream()
#define LOG_TO_STRING_WARNING(message) flare::log::null_stream()
#endif

#if FLARE_STRIP_LOG <= 4
#define COMPACT_FLARE_LOG_ERROR flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_ERROR)
#define LOG_TO_STRING_ERROR(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_ERROR, message)
#else
#define COMPACT_FLARE_LOG_ERROR flare::log::null_stream()
#define LOG_TO_STRING_ERROR(message) flare::log::null_stream()
#endif

#if FLARE_STRIP_LOG <= 5
#define COMPACT_FLARE_LOG_FATAL flare::log::log_message_fatal( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_FATAL(message) flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_FATAL, message)
#else
#define COMPACT_FLARE_LOG_FATAL flare::log::null_stream_fatal()
#define LOG_TO_STRING_FATAL(message) flare::log::null_stream_fatal()
#endif

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

// For DFATAL, we want to use log_message (as opposed to
// log_message_fatal), to be consistent with the original behavior.
#if !DCHECK_IS_ON()
#define COMPACT_FLARE_LOG_DFATAL COMPACT_FLARE_LOG_ERROR
#elif FLARE_STRIP_LOG <= 3
#define COMPACT_FLARE_LOG_DFATAL flare::log::log_message( \
      __FILE__, __LINE__, flare::log::FLARE_FATAL)
#else
#define COMPACT_FLARE_LOG_DFATAL flare::log::null_stream_fatal()
#endif

#define SYSLOG_TRACE(counter) \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_TRACE, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_DEBUG(counter) \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_DEBUG, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_INFO(counter) \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_INFO, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_WARNING(counter)  \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_WARNING, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_ERROR(counter)  \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_ERROR, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_FATAL(counter) \
  flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_FATAL, counter, \
  &flare::log::log_message::send_to_syslog_and_log)
#define SYSLOG_DFATAL(counter) \
  flare::log::log_message(__FILE__, __LINE__, flare::log::DFATAL_LEVEL, counter, \
  &flare::log::log_message::send_to_syslog_and_log)

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of log_message which seems to avoid the problem.
#define LOG(severity) COMPACT_FLARE_LOG_ ## severity.stream()
#define SYSLOG(severity) SYSLOG_ ## severity(0).stream()

namespace flare::log {

    // They need the definitions of integer types.

    // Initialize flare::log's logging library. You will see the program name
    // specified by argv0 in log outputs.
    FLARE_EXPORT void init_logging(const char *argv0);

    // Shutdown flare::log's logging library.
    FLARE_EXPORT void shutdown_logging();

    // Install a function which will be called after LOG(FATAL).
    FLARE_EXPORT void InstallFailureFunction(void (*fail_func)());

    // Enable/Disable old log cleaner.
    FLARE_EXPORT void enable_log_cleaner(int overdue_days);

    FLARE_EXPORT void disable_log_cleaner();

    FLARE_EXPORT void SetApplicationFingerprint(const std::string &fingerprint);

    class log_sink;  // defined below

    // If a non-NULL sink pointer is given, we push this message to that sink.
    // For LOG_TO_SINK we then do normal LOG(severity) logging as well.
    // This is useful for capturing messages and passing/storing them
    // somewhere more specific than the global log of the process.
    // Argument types:
    //   log_sink* sink;
    //   log_severity severity;
    // The cast is to disambiguate NULL arguments.
#define LOG_TO_SINK(sink, severity) \
  flare::log::log_message(                                    \
      __FILE__, __LINE__,                                               \
      flare::log::FLARE_ ## severity,                         \
      static_cast<flare::log::log_sink*>(sink), true).stream()
#define LOG_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                  \
  flare::log::log_message(                                    \
      __FILE__, __LINE__,                                               \
      flare::log::FLARE_ ## severity,                         \
      static_cast<flare::log::log_sink*>(sink), false).stream()

// If a non-NULL string pointer is given, we write this message to that string.
// We then do normal LOG(severity) logging as well.
// This is useful for capturing messages and storing them somewhere more
// specific than the global log of the process.
// Argument types:
//   string* message;
//   log_severity severity;
// The cast is to disambiguate NULL arguments.
// NOTE: LOG(severity) expands to log_message().stream() for the specified
// severity.
#define LOG_TO_STRING(severity, message) \
  LOG_TO_STRING_##severity(static_cast<std::string*>(message)).stream()

// If a non-NULL pointer is given, we push the message onto the end
// of a vector of strings; otherwise, we report it with LOG(severity).
// This is handy for capturing messages and perhaps passing them back
// to the caller, rather than reporting them immediately.
// Argument types:
//   log_severity severity;
//   vector<string> *outvec;
// The cast is to disambiguate NULL arguments.
#define LOG_STRING(severity, outvec) \
  LOG_TO_STRING_##severity(static_cast<std::vector<std::string>*>(outvec)).stream()

#define LOG_IF(severity, condition) \
  static_cast<void>(0),             \
  !(condition) ? (void) 0 : flare::log::log_message_voidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) \
  static_cast<void>(0),                \
  !(condition) ? (void) 0 : flare::log::log_message_voidify() & SYSLOG(severity)

#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by DCHECK_IS_ON(), so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition)  \
      LOG_IF(FATAL, FLARE_UNLIKELY(!(condition))) \
             << "Check failed: " #condition " "

    // A container for a string pointer which can be evaluated to a bool -
    // true iff the pointer is NULL.
    struct CheckOpString {
        CheckOpString(std::string *str) : str_(str) {}

        // No destructor: if str_ is non-NULL, we're about to LOG(FATAL),
        // so there's no point in cleaning up str_.
        operator bool() const {
            return FLARE_UNLIKELY(str_ != NULL);
        }

        std::string *str_;
    };

    // Function is overloaded for integral types to allow static const
    // integrals declared in classes and not defined to be used as arguments to
    // CHECK* macros. It's not encouraged though.
    template<class T>
    inline const T &GetReferenceableValue(const T &t) { return t; }

    inline char GetReferenceableValue(char t) { return t; }

    inline unsigned char GetReferenceableValue(unsigned char t) { return t; }

    inline signed char GetReferenceableValue(signed char t) { return t; }

    inline short GetReferenceableValue(short t) { return t; }

    inline unsigned short GetReferenceableValue(unsigned short t) { return t; }

    inline int GetReferenceableValue(int t) { return t; }

    inline unsigned int GetReferenceableValue(unsigned int t) { return t; }

    inline long GetReferenceableValue(long t) { return t; }

    inline unsigned long GetReferenceableValue(unsigned long t) { return t; }

    inline long long GetReferenceableValue(long long t) { return t; }

    inline unsigned long long GetReferenceableValue(unsigned long long t) {
        return t;
    }

// This is a dummy class to define the following operator.
    struct DummyClassToDefineOperator {
    };

}  // namespace flare::log

// Define global operator<< to declare using ::operator<<.
// This declaration will allow use to use CHECK macros for user
// defined classes which have operator<< (e.g., stl_logging.h).
inline std::ostream &operator<<(
        std::ostream &out, const flare::log::DummyClassToDefineOperator &) {
    return out;
}

namespace flare::log {

    // This formats a value for a failing CHECK_XX statement.  Ordinarily,
    // it uses the definition for operator<<, with a few special cases below.
    template<typename T>
    inline void make_check_op_value_string(std::ostream *os, const T &v) {
        (*os) << v;
    }

    // Overrides for char types provide readable values for unprintable
    // characters.
    template<>
    FLARE_EXPORT
    void make_check_op_value_string(std::ostream *os, const char &v);

    template<>
    FLARE_EXPORT
    void make_check_op_value_string(std::ostream *os, const signed char &v);

    template<>
    FLARE_EXPORT
    void make_check_op_value_string(std::ostream *os, const unsigned char &v);

    // This is required because nullptr is only present in c++ 11 and later.
    // Provide printable value for nullptr_t
    template<>
    FLARE_EXPORT
    void make_check_op_value_string(std::ostream *os, const std::nullptr_t &v);

    // Build the error message string. Specify no inlining for code size.
    template<typename T1, typename T2>
    FLARE_NO_INLINE std::string *MakeCheckOpString(const T1 &v1, const T2 &v2, const char *exprtext);


    namespace base {
        namespace internal {

            // If "s" is less than base_logging::INFO, returns base_logging::INFO.
            // If "s" is greater than base_logging::FATAL, returns
            // base_logging::ERROR.  Otherwise, returns "s".
            log_severity NormalizeSeverity(log_severity s);

        }  // namespace internal

        // A helper class for formatting "expr (V1 vs. V2)" in a CHECK_XX
        // statement.  See MakeCheckOpString for sample usage.  Other
        // approaches were considered: use of a template method (e.g.,
        // base::BuildCheckOpString(exprtext, base::Print<T1>, &v1,
        // base::Print<T2>, &v2), however this approach has complications
        // related to volatile arguments and function-pointer arguments).
        class FLARE_EXPORT CheckOpMessageBuilder {
        public:
            // Inserts "exprtext" and " (" to the stream.
            explicit CheckOpMessageBuilder(const char *exprtext);

            // Deletes "stream_".
            ~CheckOpMessageBuilder();

            // For inserting the first variable.
            std::ostream *ForVar1() { return stream_; }

            // For inserting the second variable (adds an intermediate " vs. ").
            std::ostream *ForVar2();

            // Get the result (inserts the closing ")").
            std::string *NewString();

        private:
            std::ostringstream *stream_;
        };

    }  // namespace base

    template<typename T1, typename T2>
    std::string *MakeCheckOpString(const T1 &v1, const T2 &v2, const char *exprtext) {
        base::CheckOpMessageBuilder comb(exprtext);
        make_check_op_value_string(comb.ForVar1(), v1);
        make_check_op_value_string(comb.ForVar2(), v2);
        return comb.NewString();
    }

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <typename T1, typename T2> \
  inline std::string* name##Impl(const T1& v1, const T2& v2,    \
                            const char* exprtext) { \
    if (FLARE_LIKELY(v1 op v2)) return NULL; \
    else return MakeCheckOpString(v1, v2, exprtext); \
  } \
  inline std::string* name##Impl(int v1, int v2, const char* exprtext) { \
    return name##Impl<int, int>(v1, v2, exprtext); \
  }

    // We use the full name Check_EQ, Check_NE, etc. in case the file including
    // base/logging.h provides its own #defines for the simpler names EQ, NE, etc.
    // This happens if, for example, those are used as token names in a
    // yacc grammar.
    DEFINE_CHECK_OP_IMPL(Check_EQ, ==)  // Compilation error with CHECK_EQ(NULL, x)?
    DEFINE_CHECK_OP_IMPL(Check_NE, !=)  // Use CHECK(x == NULL) instead.
    DEFINE_CHECK_OP_IMPL(Check_LE, <=)

    DEFINE_CHECK_OP_IMPL(Check_LT, <)

    DEFINE_CHECK_OP_IMPL(Check_GE, >=)

    DEFINE_CHECK_OP_IMPL(Check_GT, >)

#undef DEFINE_CHECK_OP_IMPL

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ et al below.

#if defined(STATIC_ANALYSIS)
    // Only for static analysis tool to know that it is equivalent to assert
#define CHECK_OP_LOG(name, op, val1, val2, log) CHECK((val1) op (val2))
#elif DCHECK_IS_ON()
    // In debug mode, avoid constructing CheckOpStrings if possible,
    // to reduce the overhead of CHECK statments by 2x.
    // Real DCHECK-heavy tests have seen 1.5x speedups.

    // The meaning of "string" might be different between now and
    // when this macro gets invoked (e.g., if someone is experimenting
    // with other string implementations that get defined after this
    // file is included).  Save the current meaning now and use it
    // in the macro.
    typedef std::string _Check_string;
#define CHECK_OP_LOG(name, op, val1, val2, log_)                         \
  while (flare::log::_Check_string* _result =                \
        flare::log::Check##name##Impl(                      \
             flare::log::GetReferenceableValue(val1),        \
             flare::log::GetReferenceableValue(val2),        \
#val1 " " #op " " #val2))                                  \
    log_(__FILE__, __LINE__,                                             \
        flare::log::CheckOpString(_result)).stream()
#else
// In optimized mode, use CheckOpString to hint to compiler that
// the while condition is unlikely.
#define CHECK_OP_LOG(name, op, val1, val2, log_)                         \
  while (flare::log::CheckOpString _result =                 \
         flare::log::Check##name##Impl(                      \
             flare::log::GetReferenceableValue(val1),        \
             flare::log::GetReferenceableValue(val2),        \
             #val1 " " #op " " #val2))                                  \
    log_(__FILE__, __LINE__, _result).stream()
#endif  // STATIC_ANALYSIS, DCHECK_IS_ON()

#if FLARE_STRIP_LOG <= 3
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, flare::log::log_message_fatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, flare::log::null_stream_fatal)
#endif // STRIP_LOG <= 3

// Equality/Inequality checks - compare two values, and log a FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   CHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define CHECK_EQ(val1, val2) CHECK_OP(_EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(_NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(_LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(_LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(_GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(_GT, > , val1, val2)

// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  flare::log::check_not_null(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected) \
  FLARE_EXPORT std::string* Check##func##expected##Impl( \
      const char* s1, const char* s2, const char* names);

    DECLARE_CHECK_STROP_IMPL(strcmp, true)

    DECLARE_CHECK_STROP_IMPL(strcmp, false)

    DECLARE_CHECK_STROP_IMPL(strcasecmp, true)

    DECLARE_CHECK_STROP_IMPL(strcasecmp, false)

#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define CHECK_STROP(func, op, expected, s1, s2) \
  while (flare::log::CheckOpString _result = \
         flare::log::Check##func##expected##Impl((s1), (s2), \
                                     #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_


// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define CHECK_INDEX(I, A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B, A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define CHECK_DOUBLE_EQ(val1, val2)              \
  do {                                           \
    CHECK_LE((val1), (val2)+0.000000000000001L); \
    CHECK_GE((val1), (val2)-0.000000000000001L); \
  } while (0)

#define CHECK_NEAR(val1, val2, margin)           \
  do {                                           \
    CHECK_LE((val1), (val2)+(margin));           \
    CHECK_GE((val1), (val2)-(margin));           \
  } while (0)

// perror()..googly style!
//
// PLOG() and PLOG_IF() and PCHECK() behave exactly like their LOG* and
// CHECK equivalents with the addition that they postpend a description
// of the current state of errno to their output lines.

#define PLOG(severity) FLARE_PLOG(severity, 0).stream()

#define FLARE_PLOG(severity, counter)  \
  flare::log::errno_log_message( \
      __FILE__, __LINE__, flare::log::FLARE_ ## severity, counter, \
      &flare::log::log_message::send_to_log)

#define PLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  !(condition) ? (void) 0 : flare::log::log_message_voidify() & PLOG(severity)

// A CHECK() macro that postpends errno if the condition is false. E.g.
//
// if (poll(fds, nfds, timeout) == -1) { PCHECK(errno == EINTR); ... }
#define PCHECK(condition)  \
      PLOG_IF(FATAL, FLARE_UNLIKELY(!(condition))) \
              << "Check failed: " #condition " "

// A CHECK() macro that lets you assert the success of a function that
// returns -1 and sets errno in case of an error. E.g.
//
// CHECK_ERR(mkdir(path, 0700));
//
// or
//
// int fd = open(filename, flags); CHECK_ERR(fd) << ": open " << filename;
#define CHECK_ERR(invocation)                                          \
PLOG_IF(FATAL, FLARE_UNLIKELY((invocation) == -1))    \
        << #invocation

// Use macro expansion to create, for each use of LOG_EVERY_N(), static
// variables with the __LINE__ expansion as part of the variable name.
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line

#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#if FLARE_COMPILER_HAS_FEATURE(thread_sanitizer) || __SANITIZE_THREAD__
#define FLARE_SANITIZE_THREAD 1
#endif


#if defined(FLARE_SANITIZE_THREAD)
    } // namespace flare::log

    // We need to identify the static variables as "benign" races
    // to avoid noisy reports from TSAN.
    extern "C" void AnnotateBenignRaceSized(
      const char *file,
      int line,
      const volatile void *mem,
      long size,
      const char *description);

    namespace flare::log {
#endif

#define SOME_KIND_OF_LOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    flare::log::log_message( \
        __FILE__, __LINE__, flare::log::FLARE_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (condition && \
      ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
    flare::log::log_message( \
        __FILE__, __LINE__, flare::log::FLARE_ ## severity, LOG_OCCURRENCES, \
                 &what_to_do).stream()

#define SOME_KIND_OF_PLOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    flare::log::errno_log_message( \
        __FILE__, __LINE__, flare::log::FLARE_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_FIRST_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0); \
  FLARE_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  if (LOG_OCCURRENCES <= n) \
    ++LOG_OCCURRENCES; \
  if (LOG_OCCURRENCES <= n) \
    flare::log::log_message( \
        __FILE__, __LINE__, flare::log::FLARE_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

    namespace log_internal {
        template<bool>
        struct compile_assert {
        };
        struct crash_reason;

// Returns true if FailureSignalHandler is installed.
// Needs to be exported since it's used by the signalhandler_unittest.
        FLARE_EXPORT bool IsFailureSignalHandlerInstalled();
    }  // namespace log_internal

#define LOG_EVERY_N(severity, n)                                        \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), flare::log::log_message::send_to_log)

#define SYSLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), flare::log::log_message::send_to_syslog_and_log)

#define PLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_PLOG_EVERY_N(severity, (n), flare::log::log_message::send_to_log)

#define LOG_FIRST_N(severity, n) \
  SOME_KIND_OF_LOG_FIRST_N(severity, (n), flare::log::log_message::send_to_log)

#define LOG_IF_EVERY_N(severity, condition, n) \
  SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n), flare::log::log_message::send_to_log)

// We want the special COUNTER value available for LOG_EVERY_X()'ed messages
    enum PRIVATE_Counter {
        COUNTER
    };

#ifdef FLARE_LOG_NO_ABBREVIATED_SEVERITIES
    // wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
    // substituted with 0, and it expands to COMPACT_FLARE_LOG_0. To allow us
    // to keep using this syntax, we define this macro to do the same thing
    // as COMPACT_FLARE_LOG_ERROR.
#define COMPACT_FLARE_LOG_0 COMPACT_FLARE_LOG_ERROR
#define SYSLOG_0 SYSLOG_ERROR
#define LOG_TO_STRING_0 LOG_TO_STRING_ERROR
    // Needed for LOG_IS_ON(ERROR).
    const log_severity FLARE_LOG_0 = FLARE_ERROR;
#else
// Users may include windows.h after logging.h without
// FLARE_LOG_NO_ABBREVIATED_SEVERITIES nor WIN32_LEAN_AND_MEAN.
// For this case, we cannot detect if ERROR is defined before users
// actually use ERROR. Let's make an undefined symbol to warn users.
# define FLARE_LOG_ERROR_MSG ERROR_macro_is_defined_Define_FLARE_LOG_NO_ABBREVIATED_SEVERITIES_before_including_logging_h_See_the_document_for_detail
# define COMPACT_FLARE_LOG_0 FLARE_LOG_ERROR_MSG
# define SYSLOG_0 FLARE_LOG_ERROR_MSG
# define LOG_TO_STRING_0 FLARE_LOG_ERROR_MSG
# define FLARE_LOG_0 FLARE_LOG_ERROR_MSG
#endif

// Plus some debug-logging macros that get compiled to nothing for production

#if DCHECK_IS_ON()

#define DLOG(severity) LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) \
  LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

    // debug-only checking.  executed if DCHECK_IS_ON().
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)

#else  // !DCHECK_IS_ON()

#define DLOG(severity)  \
  static_cast<void>(0), \
  true ? (void) 0 : flare::log::log_message_voidify() & LOG(severity)

#define DVLOG(verboselevel)             \
  static_cast<void>(0),                 \
  (true || !VLOG_IS_ON(verboselevel)) ? \
      (void) 0 : flare::log::log_message_voidify() & LOG(INFO)

#define DLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  (true || !(condition)) ? (void) 0 : flare::log::log_message_voidify() & LOG(severity)

#define DLOG_EVERY_N(severity, n) \
  static_cast<void>(0),           \
  true ? (void) 0 : flare::log::log_message_voidify() & LOG(severity)

#define DLOG_IF_EVERY_N(severity, condition, n) \
  static_cast<void>(0),                         \
  (true || !(condition))? (void) 0 : flare::log::log_message_voidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
  static_cast<void>(0),        \
  true ? (void) 0 : LOG_ASSERT(condition)

// MSVC warning C4127: conditional expression is constant
#define DCHECK(condition) \
  while (false) \
    CHECK(condition)

#define DCHECK_EQ(val1, val2) \
  while (false) \
    CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
  while (false) \
    CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
  while (false) \
    CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
  while (false) \
    CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
  while (false) \
    CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
  while (false) \
    CHECK_GT(val1, val2)

// You may see warnings in release mode if you don't use the return
// value of DCHECK_NOTNULL. Please just use DCHECK for such cases.
#define DCHECK_NOTNULL(val) (val)

#define DCHECK_STREQ(str1, str2) \
  while (false) \
    CHECK_STREQ(str1, str2)

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false) \
    CHECK_STRCASEEQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
  while (false) \
    CHECK_STRNE(str1, str2)

#define DCHECK_STRCASENE(str1, str2) \
  while (false) \
    CHECK_STRCASENE(str1, str2)

#endif  // DCHECK_IS_ON()

// Log only in verbose mode.

#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))

#define VLOG_IF(verboselevel, condition) \
  LOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#define VLOG_EVERY_N(verboselevel, n) \
  LOG_IF_EVERY_N(INFO, VLOG_IS_ON(verboselevel), n)

#define VLOG_IF_EVERY_N(verboselevel, condition, n) \
  LOG_IF_EVERY_N(INFO, (condition) && VLOG_IS_ON(verboselevel), n)

    namespace base_logging {

        // log_message::log_stream is a std::ostream backed by this streambuf.
        // This class ignores overflow and leaves two bytes at the end of the
        // buffer to allow for a '\n' and '\0'.
        class FLARE_EXPORT LogStreamBuf : public std::streambuf {
        public:
            // REQUIREMENTS: "len" must be >= 2 to account for the '\n' and '\0'.
            LogStreamBuf(char *buf, int len) {
                setp(buf, buf + len - 2);
            }

            // This effectively ignores overflow.
            int_type overflow(int_type ch) {
                return ch;
            }

            // Legacy public ostrstream method.
            size_t pcount() const { return pptr() - pbase(); }

            char *pbase() const { return std::streambuf::pbase(); }
        };

    }  // namespace base_logging

    //
    // This class more or less represents a particular log message.  You
    // create an instance of log_message and then stream stuff to it.
    // When you finish streaming to it, ~log_message is called and the
    // full message gets streamed to the appropriate destination.
    //
    // You shouldn't actually use log_message's constructor to log things,
    // though.  You should use the LOG() macro (and variants thereof)
    // above.
    class FLARE_EXPORT log_message {
    public:
        enum {
            // Passing kNoLogPrefix for the line number disables the
            // log-message prefix. Useful for using the log_message
            // infrastructure as a printing utility. See also the --log_prefix
            // flag for controlling the log-message prefix on an
            // application-wide basis.
            kNoLogPrefix = -1
        };

        // log_stream inherit from non-DLL-exported class (std::ostrstream)
        // and VC++ produces a warning for this situation.
        // However, MSDN says "C4275 can be ignored in Microsoft Visual C++
        // 2005 if you are deriving from a type in the Standard C++ Library"
        // http://msdn.microsoft.com/en-us/library/3tdb471s(VS.80).aspx
        // Let's just ignore the warning.
        class FLARE_EXPORT log_stream : public std::ostream {
        public:
            log_stream(char *buf, int len, uint64_t ctr)
                    : std::ostream(NULL),
                      streambuf_(buf, len),
                      ctr_(ctr),
                      self_(this) {
                rdbuf(&streambuf_);
            }

            uint64_t ctr() const { return ctr_; }

            void set_ctr(uint64_t ctr) { ctr_ = ctr; }

            log_stream *self() const { return self_; }

            // Legacy std::streambuf methods.
            size_t pcount() const { return streambuf_.pcount(); }

            char *pbase() const { return streambuf_.pbase(); }

            char *str() const { return pbase(); }

        private:
            log_stream(const log_stream &);

            log_stream &operator=(const log_stream &);

            base_logging::LogStreamBuf streambuf_;
            uint64_t ctr_;  // Counter hack (for the LOG_EVERY_X() macro)
            log_stream *self_;  // Consistency check hack
        };

    public:
        // icc 8 requires this typedef to avoid an internal compiler error.
        typedef void (log_message::*SendMethod)();

        log_message(const char *file, int line, log_severity severity, uint64_t ctr,
                   SendMethod send_method);

        // Two special constructors that generate reduced amounts of code at
        // LOG call sites for common cases.

        // Used for LOG(INFO): Implied are:
        // severity = INFO, ctr = 0, send_method = &log_message::send_to_log.
        //
        // Using this constructor instead of the more complex constructor above
        // saves 19 bytes per call site.
        log_message(const char *file, int line);

        // Used for LOG(severity) where severity != INFO.  Implied
        // are: ctr = 0, send_method = &log_message::send_to_log
        //
        // Using this constructor instead of the more complex constructor above
        // saves 17 bytes per call site.
        log_message(const char *file, int line, log_severity severity);

        // Constructor to log this message to a specified sink (if not NULL).
        // Implied are: ctr = 0, send_method = &log_message::send_to_sink_and_log if
        // also_send_to_log is true, send_method = &log_message::send_to_sink otherwise.
        log_message(const char *file, int line, log_severity severity, log_sink *sink,
                   bool also_send_to_log);

        // Constructor where we also give a vector<string> pointer
        // for storing the messages (if the pointer is not NULL).
        // Implied are: ctr = 0, send_method = &log_message::save_or_send_to_log.
        log_message(const char *file, int line, log_severity severity,
                   std::vector<std::string> *outvec);

        // Constructor where we also give a string pointer for storing the
        // message (if the pointer is not NULL).  Implied are: ctr = 0,
        // send_method = &log_message::write_to_string_and_log.
        log_message(const char *file, int line, log_severity severity,
                   std::string *message);

        // A special constructor used for check failures
        log_message(const char *file, int line, const CheckOpString &result);

        ~log_message();

        // Flush a buffered message to the sink set in the constructor.  Always
        // called by the destructor, it may also be called from elsewhere if
        // needed.  Only the first call is actioned; any later ones are ignored.
        void flush();

        // An arbitrary limit on the length of a single log message.  This
        // is so that streaming can be done more efficiently.
        static const size_t kMaxLogMessageLen;

        // Theses should not be called directly outside of logging.*,
        // only passed as SendMethod arguments to other log_message methods:
        void send_to_log();  // Actually dispatch to the logs
        void send_to_syslog_and_log();  // Actually dispatch to syslog and the logs

        // Call abort() or similar to perform LOG(FATAL) crash.
        static void fail();

        std::ostream &stream();

        int preserved_errno() const;

        // Must be called without the log_mutex held.  (L < log_mutex)
        static int64_t num_messages(int severity);

        struct log_message_data;

    private:
        // Fully internal SendMethod cases:
        void send_to_sink_and_log();  // Send to sink if provided and dispatch to the logs
        void send_to_sink();  // Send to sink if provided, do nothing otherwise.

        // Write to string if provided and dispatch to the logs.
        void write_to_string_and_log();

        void save_or_send_to_log();  // Save to stringvec if provided, else to logs

        void init(const char *file, int line, log_severity severity,
                  void (log_message::*send_method)());

        // Used to fill in crash information during LOG(FATAL) failures.
        void record_crash_reason(log_internal::crash_reason *reason);

        // Counts of messages sent at each priority:
        static int64_t num_messages_[NUM_SEVERITIES];  // under log_mutex

        // We keep the data in a separate struct so that each instance of
        // log_message uses less stack space.
        log_message_data *allocated_;
        log_message_data *data_;

        friend class log_destination;

        log_message(const log_message &);

        void operator=(const log_message &);
    };

    // This class happens to be thread-hostile because all instances share
    // a single data buffer, but since it can only be created just before
    // the process dies, we don't worry so much.
    class FLARE_EXPORT log_message_fatal : public log_message {
    public:
        log_message_fatal(const char *file, int line);

        log_message_fatal(const char *file, int line, const CheckOpString &result);

        ~log_message_fatal();
    };

    // A non-macro interface to the log facility; (useful
    // when the logging level is not a compile-time constant).
    inline void LogAtLevel(int const severity, std::string const &msg) {
        log_message(__FILE__, __LINE__, severity).stream() << msg;
    }

// A macro alternative of LogAtLevel. New code may want to use this
// version since there are two advantages: 1. this version outputs the
// file name and the line number where this macro is put like other
// LOG macros, 2. this macro can be used as C++ stream.
#define LOG_AT_LEVEL(severity) flare::log::log_message(__FILE__, __LINE__, severity).stream()


    // Helper for CHECK_NOTNULL().
    //
    // In C++11, all cases can be handled by a single function. Since the value
    // category of the argument is preserved (also for rvalue references),
    // member initializer lists like the one below will compile correctly:
    //
    //   Foo()
    //     : x_(CHECK_NOTNULL(MethodReturningUniquePtr())) {}
    template<typename T>
    T check_not_null(const char *file, int line, const char *names, T &&t) {
        if (t == nullptr) {
            log_message_fatal(file, line, new std::string(names));
        }
        return std::forward<T>(t);
    }


    // Allow folks to put a counter in the LOG_EVERY_X()'ed messages. This
    // only works if ostream is a log_stream. If the ostream is not a
    // log_stream you'll get an assert saying as much at runtime.
    FLARE_EXPORT std::ostream &operator<<(std::ostream &os,
                                          const PRIVATE_Counter &);


    // Derived class for PLOG*() above.
    class FLARE_EXPORT errno_log_message : public log_message {
    public:

        errno_log_message(const char *file, int line, log_severity severity, uint64_t ctr,
                        void (log_message::*send_method)());

        // Postpends ": strerror(errno) [errno]".
        ~errno_log_message();

    private:
        errno_log_message(const errno_log_message &);

        void operator=(const errno_log_message &);
    };


    // This class is used to explicitly ignore values in the conditional
    // logging macros.  This avoids compiler warnings like "value computed
    // is not used" and "statement has no effect".

    class FLARE_EXPORT log_message_voidify {
    public:
        log_message_voidify() {}

        // This has to be an operator with a precedence lower than << but
        // higher than ?:
        void operator&(std::ostream &) {}
    };


    // Flushes all log files that contains messages that are at least of
    // the specified severity level.  Thread-safe.
    FLARE_EXPORT void flush_log_files(log_severity min_severity);

    // Flushes all log files that contains messages that are at least of
    // the specified severity level. Thread-hostile because it ignores
    // locking -- used for catastrophic failures.
    FLARE_EXPORT void flush_log_files_unsafe(log_severity min_severity);

    //
    // Set the destination to which a particular severity level of log
    // messages is sent.  If base_filename is "", it means "don't log this
    // severity".  Thread-safe.
    //
    FLARE_EXPORT void set_log_destination(log_severity severity,
                                        const char *base_filename);

    //
    // Set the basename of the symlink to the latest log file at a given
    // severity.  If symlink_basename is empty, do not make a symlink.  If
    // you don't call this function, the symlink basename is the
    // invocation name of the program.  Thread-safe.
    //
    FLARE_EXPORT void set_log_symlink(log_severity severity,
                                    const char *symlink_basename);

    //
    // Used to send logs to some other kind of destination
    // Users should subclass log_sink and override send to do whatever they want.
    // Implementations must be thread-safe because a shared instance will
    // be called from whichever thread ran the LOG(XXX) line.
    class FLARE_EXPORT log_sink {
    public:
        virtual ~log_sink();

        // Sink's logging logic (message_len is such as to exclude '\n' at the end).
        // This method can't use LOG() or CHECK() as logging system mutex(s) are held
        // during this call.
        virtual void send(log_severity severity, const char *full_filename,
                          const char *base_filename, int line,
                          const struct ::tm *tm_time,
                          const char *message, size_t message_len, int32_t /*usecs*/) {
            send(severity, full_filename, base_filename, line,
                 tm_time, message, message_len);
        }

        // This send() signature is obsolete.
        // New implementations should define this in terms of
        // the above send() method.
        virtual void send(log_severity severity, const char *full_filename,
                          const char *base_filename, int line,
                          const struct ::tm *tm_time,
                          const char *message, size_t message_len) = 0;

        // Redefine this to implement waiting for
        // the sink's logging logic to complete.
        // It will be called after each send() returns,
        // but before that log_message exits or crashes.
        // By default this function does nothing.
        // Using this function one can implement complex logic for send()
        // that itself involves logging; and do all this w/o causing deadlocks and
        // inconsistent rearrangement of log messages.
        // E.g. if a log_sink has thread-specific actions, the send() method
        // can simply add the message to a queue and wake up another thread that
        // handles real logging while itself making some LOG() calls;
        // WaitTillSent() can be implemented to wait for that logic to complete.
        // See our unittest for an example.
        virtual void WaitTillSent();

        // Returns the normal text output of the log message.
        // Can be useful to implement send().
        static std::string ToString(log_severity severity, const char *file, int line,
                                    const struct ::tm *tm_time,
                                    const char *message, size_t message_len,
                                    int32_t usecs);

        // Obsolete
        static std::string ToString(log_severity severity, const char *file, int line,
                                    const struct ::tm *tm_time,
                                    const char *message, size_t message_len) {
            return ToString(severity, file, line, tm_time, message, message_len, 0);
        }
    };

    // Add or remove a log_sink as a consumer of logging data.  Thread-safe.
    FLARE_EXPORT void add_log_sink(log_sink *destination);

    FLARE_EXPORT void remove_log_sink(log_sink *destination);

    //
    // Specify an "extension" added to the filename specified via
    // set_log_destination.  This applies to all severity levels.  It's
    // often used to append the port we're listening on to the logfile
    // name.  Thread-safe.
    //
    FLARE_EXPORT void set_log_filename_extension(
            const char *filename_extension);

    //
    // Make it so that all log messages of at least a particular severity
    // are logged to stderr (in addition to logging to the usual log
    // file(s)).  Thread-safe.
    //
    FLARE_EXPORT void set_stderr_logging(log_severity min_severity);

    //
    // Make it so that all log messages go only to stderr.  Thread-safe.
    //
    FLARE_EXPORT void log_to_stderr();

    //
    // Make it so that all log messages of at least a particular severity are
    // logged via email to a list of addresses (in addition to logging to the
    // usual log file(s)).  The list of addresses is just a string containing
    // the email addresses to send to (separated by spaces, say).  Thread-safe.
    //
    FLARE_EXPORT void set_email_logging(log_severity min_severity,
                                      const char *addresses);

    // A simple function that sends email. dest is a commma-separated
    // list of addressess.  Thread-safe.
    FLARE_EXPORT bool SendEmail(const char *dest,
                                const char *subject, const char *body);

    FLARE_EXPORT const std::vector<std::string> &GetLoggingDirectories();

    // For tests only:  Clear the internal [cached] list of logging directories to
    // force a refresh the next time GetLoggingDirectories is called.
    // Thread-hostile.
    void TestOnly_ClearLoggingDirectoriesList();

    // Returns a set of existing temporary directories, which will be a
    // subset of the directories returned by GetLogginDirectories().
    // Thread-safe.
    FLARE_EXPORT void GetExistingTempDirectories(std::vector<std::string> *list);

    // Print any fatal message again -- useful to call from signal handler
    // so that the last thing in the output is the fatal message.
    // Thread-hostile, but a race is unlikely.
    FLARE_EXPORT void reprint_fatal_message();

    // Truncate a log file that may be the append-only output of multiple
    // processes and hence can't simply be renamed/reopened (typically a
    // stdout/stderr).  If the file "path" is > "limit" bytes, copy the
    // last "keep" bytes to offset 0 and truncate the rest. Since we could
    // be racing with other writers, this approach has the potential to
    // lose very small amounts of data. For security, only follow symlinks
    // if the path is /proc/self/fd/*
    FLARE_EXPORT void truncate_log_file(const char *path,
                                      int64_t limit, int64_t keep);

    // Truncate stdout and stderr if they are over the value specified by
    // --max_log_size; keep the final 1MB.  This function has the same
    // race condition as truncate_log_file.
    FLARE_EXPORT void truncate_stdout_stderr();

    // Return the string representation of the provided log_severity level.
    // Thread-safe.
    FLARE_EXPORT const char *GetLogSeverityName(log_severity severity);

    // ---------------------------------------------------------------------
    // Implementation details that are not useful to most clients
    // ---------------------------------------------------------------------

    // A inner_logger is the interface used by logging modules to emit entries
    // to a log.  A typical implementation will dump formatted data to a
    // sequence of files.  We also provide interfaces that will forward
    // the data to another thread so that the invoker never blocks.
    // Implementations should be thread-safe since the logging system
    // will write to them from multiple threads.

    namespace base {

        class FLARE_EXPORT inner_logger {
        public:
            virtual ~inner_logger();

            // Writes "message[0,message_len-1]" corresponding to an event that
            // occurred at "timestamp".  If "force_flush" is true, the log file
            // is flushed immediately.
            //
            // The input message has already been formatted as deemed
            // appropriate by the higher level logging facility.  For example,
            // textual log messages already contain timestamps, and the
            // file:linenumber header.
            virtual void write(bool force_flush,
                               time_t timestamp,
                               const char *message,
                               int message_len) = 0;

            // Flush any buffered messages
            virtual void flush() = 0;

            // Get the current LOG file size.
            // The returned value is approximate since some
            // logged data may not have been flushed to disk yet.
            virtual uint32_t log_size() = 0;
        };

        // Get the logger for the specified severity level.  The logger
        // remains the property of the logging module and should not be
        // deleted by the caller.  Thread-safe.
        extern FLARE_EXPORT inner_logger *get_logger(log_severity level);

        // Set the logger for the specified severity level.  The logger
        // becomes the property of the logging module and should not
        // be deleted by the caller.  Thread-safe.
        extern FLARE_EXPORT void set_logger(log_severity level, inner_logger *l);

    }

    // glibc has traditionally implemented two incompatible versions of
    // strerror_r(). There is a poorly defined convention for picking the
    // version that we want, but it is not clear whether it even works with
    // all versions of glibc.
    // So, instead, we provide this wrapper that automatically detects the
    // version that is in use, and then implements POSIX semantics.
    // N.B. In addition to what POSIX says, we also guarantee that "buf" will
    // be set to an empty string, if this function failed. This means, in most
    // cases, you do not need to check the error code and you can directly
    // use the value of "buf". It will never have an undefined value.
    // DEPRECATED: Use StrError(int) instead.
    FLARE_EXPORT int posix_strerror_r(int err, char *buf, size_t len);

    // A thread-safe replacement for strerror(). Returns a string describing the
    // given POSIX error code.
    FLARE_EXPORT std::string StrError(int err);

    // A class for which we define operator<<, which does nothing.
    class FLARE_EXPORT null_stream : public log_message::log_stream {
    public:
        // Initialize the log_stream so the messages can be written somewhere
        // (they'll never be actually displayed). This will be needed if a
        // null_stream& is implicitly converted to log_stream&, in which case
        // the overloaded null_stream::operator<< will not be invoked.
        null_stream() : log_message::log_stream(message_buffer_, 1, 0) {}

        null_stream(const char * /*file*/, int /*line*/,
                   const CheckOpString & /*result*/) :
                log_message::log_stream(message_buffer_, 1, 0) {}

        null_stream &stream() { return *this; }

    private:
        // A very short buffer for messages (which we discard anyway). This
        // will be needed if null_stream& converted to log_stream& (e.g. as a
        // result of a conditional expression).
        char message_buffer_[2]{0,0};
    };

    // Do nothing. This operator is inline, allowing the message to be
    // compiled away. The message will not be compiled away if we do
    // something like (flag ? LOG(INFO) : LOG(ERROR)) << message; when
    // SKIP_LOG=WARNING. In those cases, null_stream will be implicitly
    // converted to log_stream and the message will be computed and then
    // quietly discarded.
    template<class T>
    inline null_stream &operator<<(null_stream &str, const T &) { return str; }

    // Similar to null_stream, but aborts the program (without stack
    // trace), like log_message_fatal.
    class FLARE_EXPORT null_stream_fatal : public null_stream {
    public:
        null_stream_fatal() {}

        null_stream_fatal(const char *file, int line, const CheckOpString &result) :
                null_stream(file, line, result) {}

        ~null_stream_fatal() throw() { _exit(1); }
    };

    // Install a signal handler that will dump signal information and a stack
    // trace when the program crashes on certain signals.  We'll install the
    // signal handler for the following signals.
    //
    // SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGBUS, and SIGTERM.
    //
    // By default, the signal handler will write the failure dump to the
    // standard error.  You can customize the destination by installing your
    // own writer function by InstallFailureWriter() below.
    //
    // Note on threading:
    //
    // The function should be called before threads are created, if you want
    // to use the failure signal handler for all threads.  The stack trace
    // will be shown only for the thread that receives the signal.  In other
    // words, stack traces of other threads won't be shown.
    FLARE_EXPORT void InstallFailureSignalHandler();

    // Installs a function that is used for writing the failure dump.  "data"
    // is the pointer to the beginning of a message to be written, and "size"
    // is the size of the message.  You should not expect the data is
    // terminated with '\0'.
    FLARE_EXPORT void InstallFailureWriter(
            void (*writer)(const char *data, int size));

}  // flare::log

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
    flare::log::log_message(file, line, flare::log::severity).stream()


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

// Print a log for at most once.
// Almost zero overhead when the log was printed.
#ifndef LOG_ONCE
# define LOG_ONCE(severity) LOG_FIRST_N(severity, 1)
# define LOG_IF_ONCE(severity, condition) LOG_IF_FIRST_N(severity, condition, 1)
#endif

// Print a log after every N calls. First call always prints.
// Each call to this macro has a cost of relaxed atomic increment.
#ifndef LOG_EVERY_N
# define LOG_EVERY_N(severity, N)                                \
     FLARE_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_EVERY_N(severity, condition, N)                  \
     FLARE_LOG_IF_EVERY_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print logs for first N calls.
// Almost zero overhead when the log was printed for N times
#ifndef LOG_FIRST_N
# define LOG_FIRST_N(severity, N)                                \
     FLARE_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, true, N)
# define LOG_IF_FIRST_N(severity, condition, N)                  \
     FLARE_LOG_IF_FIRST_N_IMPL(LOG_IF, severity, condition, N)
#endif

// Print a log every second. First call always prints.
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

/// flare logs

#define FLARE_CHECK(expr, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK(expr, ##__VA_ARGS__)
#define FLARE_CHECK_EQ(val1, val2, ...)   \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP( \
      _EQ, ==, val1, val2, ##__VA_ARGS__)  // `##` is GNU extension.
#define FLARE_CHECK_NE(val1, val2, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(_NE, !=, val1, val2, ##__VA_ARGS__)
#define FLARE_CHECK_LE(val1, val2, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(_LE, <=, val1, val2, ##__VA_ARGS__)
#define FLARE_CHECK_LT(val1, val2, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(_LT, <, val1, val2, ##__VA_ARGS__)
#define FLARE_CHECK_GE(val1, val2, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(_GE, >=, val1, val2, ##__VA_ARGS__)
#define FLARE_CHECK_GT(val1, val2, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(_GT, >, val1, val2, ##__VA_ARGS__)
#define FLARE_CHECK_NEAR(val1, val2, margin, ...)             \
  do {                                                        \
    FLARE_CHECK_LE((val1), (val2) + (margin), ##__VA_ARGS__); \
    FLARE_CHECK_GE((val1), (val2) - (margin), ##__VA_ARGS__); \
  } while (0)

// Do NOT use `DCHECK`s from glog, they're not `constexpr`-friendly.
#ifndef NDEBUG
#define FLARE_DCHECK(expr, ...) FLARE_CHECK(expr, ##__VA_ARGS__)
#define FLARE_DCHECK_EQ(expr1, expr2, ...) \
  FLARE_CHECK_EQ(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_NE(expr1, expr2, ...) \
  FLARE_CHECK_NE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_LE(expr1, expr2, ...) \
  FLARE_CHECK_LE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_GE(expr1, expr2, ...) \
  FLARE_CHECK_GE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_LT(expr1, expr2, ...) \
  FLARE_CHECK_LT(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_GT(expr1, expr2, ...) \
  FLARE_CHECK_GT(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_NEAR(expr1, expr2, margin, ...) \
  FLARE_CHECK_NEAR(expr1, expr2, ##__VA_ARGS__)
#else
#define FLARE_DCHECK(expr, ...) \
  while (0) FLARE_CHECK(expr, ##__VA_ARGS__)
#define FLARE_DCHECK_EQ(expr1, expr2, ...) \
  while (0) FLARE_CHECK_EQ(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_NE(expr1, expr2, ...) \
  while (0) FLARE_CHECK_NE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_LE(expr1, expr2, ...) \
  while (0) FLARE_CHECK_LE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_GE(expr1, expr2, ...) \
  while (0) FLARE_CHECK_GE(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_LT(expr1, expr2, ...) \
  while (0) FLARE_CHECK_LT(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_GT(expr1, expr2, ...) \
  while (0) FLARE_CHECK_GT(expr1, expr2, ##__VA_ARGS__)
#define FLARE_DCHECK_NEAR(expr1, expr2, margin, ...) \
  while (0) FLARE_CHECK_NEAR(expr1, expr2, ##__VA_ARGS__)
#endif

#define FLARE_PCHECK(expr, ...) \
  FLARE_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ##__VA_ARGS__)

#define FLARE_VLOG(n, ...)                                         \
  LOG_IF(INFO, FLARE_UNLIKELY(VLOG_IS_ON(n)))                      \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)

#define FLARE_LOG_TRACE(...)                                              \
  LOG(TRACE) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                     __VA_ARGS__)
#define FLARE_LOG_DEBUG(...)                                              \
  LOG(DEBUG) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                     __VA_ARGS__)

#define FLARE_LOG_INFO(...)                                              \
  LOG(INFO) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                     __VA_ARGS__)
#define FLARE_LOG_WARNING(...)                                              \
  LOG(WARNING) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                        __VA_ARGS__)
#define FLARE_LOG_ERROR(...)                                              \
  LOG(ERROR) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                      __VA_ARGS__)
#define FLARE_LOG_FATAL(...)                                                \
  do {                                                                      \
    LOG(FATAL) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                        __VA_ARGS__);       \
  } while (0);                                                              \
  FLARE_UNREACHABLE()

#define FLARE_LOG_INFO_IF(expr, ...)                           \
  LOG_IF(INFO, expr) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_LOG_WARNING_IF(expr, ...)                            \
  LOG_IF(WARNING, FLARE_UNLIKELY(expr))                            \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_LOG_ERROR_IF(expr, ...)                              \
  LOG_IF(ERROR, FLARE_UNLIKELY(expr))                              \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_LOG_FATAL_IF(expr, ...)                              \
  LOG_IF(FATAL, FLARE_UNLIKELY(expr))                              \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)

#define FLARE_LOG_INFO_EVERY_N(N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_EVERY_N(INFO, N, __VA_ARGS__)
#define FLARE_LOG_WARNING_EVERY_N(N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_EVERY_N(WARNING, N, __VA_ARGS__)
#define FLARE_LOG_ERROR_EVERY_N(N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_EVERY_N(ERROR, N, __VA_ARGS__)
#define FLARE_LOG_FATAL_EVERY_N(N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_EVERY_N(FATAL, N, __VA_ARGS__)

#define FLARE_LOG_INFO_IF_EVERY_N(expr, N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_IF_EVERY_N(expr, INFO, N, __VA_ARGS__)
#define FLARE_LOG_WARNING_IF_EVERY_N(expr, N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_IF_EVERY_N(expr, WARNING, N, __VA_ARGS__)
#define FLARE_LOG_ERROR_IF_EVERY_N(expr, N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_IF_EVERY_N(expr, ERROR, N, __VA_ARGS__)
#define FLARE_LOG_FATAL_IF_EVERY_N(expr, N, ...) \
  FLARE_INTERNAL_DETAIL_LOG_IF_EVERY_N(expr, FATAL, N, __VA_ARGS__)

#define FLARE_LOG_INFO_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(INFO, __VA_ARGS__)
#define FLARE_LOG_WARNING_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(WARNING, __VA_ARGS__)
#define FLARE_LOG_ERROR_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(ERROR, __VA_ARGS__)
#define FLARE_LOG_FATAL_ONCE(...)                        \
  /* You're unlikely to have a second chance anyway.. */ \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(FATAL, __VA_ARGS__)

#define FLARE_LOG_INFO_IF_ONCE(expr, ...)                \
  do {                                                   \
    if (expr) {                                          \
      FLARE_INTERNAL_DETAIL_LOG_ONCE(INFO, __VA_ARGS__); \
    }                                                    \
  } while (0)
#define FLARE_LOG_WARNING_IF_ONCE(expr, ...)                \
  do {                                                      \
    if (FLARE_UNLIKELY(expr)) {                             \
      FLARE_INTERNAL_DETAIL_LOG_ONCE(WARNING, __VA_ARGS__); \
    }                                                       \
  } while (0)
#define FLARE_LOG_ERROR_IF_ONCE(expr, ...)                \
  do {                                                    \
    if (FLARE_UNLIKELY(expr)) {                           \
      FLARE_INTERNAL_DETAIL_LOG_ONCE(ERROR, __VA_ARGS__); \
    }                                                     \
  } while (0)
#define FLARE_LOG_FATAL_IF_ONCE(expr, ...)                \
  do {                                                    \
    if (FLARE_UNLIKELY(expr)) {                           \
      FLARE_INTERNAL_DETAIL_LOG_ONCE(FATAL, __VA_ARGS__); \
    }                                                     \
  } while (0)

#define FLARE_DLOG_INFO(...)                                              \
  DLOG(INFO) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                      __VA_ARGS__)
#define FLARE_DLOG_WARNING(...)                                              \
  DLOG(WARNING) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                         __VA_ARGS__)
#define FLARE_DLOG_ERROR(...)                                              \
  DLOG(ERROR) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                       __VA_ARGS__)
#define FLARE_DLOG_FATAL(...)                                              \
  DLOG(FATAL) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                       __VA_ARGS__)

#define FLARE_DLOG_INFO_IF(expr, ...)                           \
  DLOG_IF(INFO, expr) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_DLOG_WARNING_IF(expr, ...)                           \
  DLOG_IF(WARNING, FLARE_UNLIKELY(expr))                           \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_DLOG_ERROR_IF(expr, ...)                             \
  DLOG_IF(ERROR, FLARE_UNLIKELY(expr))                             \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_DLOG_FATAL_IF(expr, ...)                             \
  DLOG_IF(FATAL, FLARE_UNLIKELY(expr))                             \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)

#define FLARE_DLOG_INFO_EVERY_N(N, ...)                           \
  DLOG_EVERY_N(INFO, N) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_DLOG_WARNING_EVERY_N(N, ...)                           \
  DLOG_EVERY_N(WARNING, N) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_DLOG_ERROR_EVERY_N(N, ...)                           \
  DLOG_EVERY_N(ERROR, N) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_DLOG_FATAL_EVERY_N(N, ...)                           \
  DLOG_EVERY_N(FATAL, N) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
#define FLARE_DLOG_INFO_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(INFO, __VA_ARGS__)
#define FLARE_DLOG_WARNING_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(WARNING, __VA_ARGS__)
#define FLARE_DLOG_ERROR_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(ERROR, __VA_ARGS__)
#define FLARE_DLOG_FATAL_ONCE(...) \
  FLARE_INTERNAL_DETAIL_LOG_ONCE(FATAL, __VA_ARGS__)
#else
// The expansion below is NOT a bug.
//
// FLARE_DLOG_XXX is expanded to "no-op" expression if `NDEBUG` is defined.
// Therefore, although `FLARE_DLOG_INFO_ONCE` doesn't behave in the same way as
// `FLARE_DLOG_INFO`, the end result it the same (nothing is ever evaluated).
//
// The reason why we expands to `FLARE_DLOG_XXX` instead of simply `(void)0` is
// that `FLARE_DLOG_XXX` does some trick (done by glog, to be precise) to avoid
// "unused variable" in a best-effort fashion, and we want to benefit from that
// here.
#define FLARE_DLOG_INFO_ONCE(...) FLARE_DLOG_INFO(__VA_ARGS__);
#define FLARE_DLOG_WARNING_ONCE(...) FLARE_DLOG_WARNING(__VA_ARGS__);
#define FLARE_DLOG_ERROR_ONCE(...) FLARE_DLOG_ERROR(__VA_ARGS__);
#define FLARE_DLOG_FATAL_ONCE(...) FLARE_DLOG_FATAL(__VA_ARGS__);
#endif

#define FLARE_PLOG_INFO(...)                                              \
  PLOG(INFO) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                      __VA_ARGS__)
#define FLARE_PLOG_WARNING(...)                                              \
  PLOG(WARNING) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                         __VA_ARGS__)
#define FLARE_PLOG_ERROR(...)                                              \
  PLOG(ERROR) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                       __VA_ARGS__)
#define FLARE_PLOG_FATAL(...)                                              \
  PLOG(FATAL) << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                       __VA_ARGS__)

#define FLARE_PLOG_INFO_IF(expr, ...)                           \
  PLOG_IF(INFO, expr) << ::flare::internal::logging::format_log( \
      __FILE__, __LINE__, __VA_ARGS__)
#define FLARE_PLOG_WARNING_IF(expr, ...)                           \
  PLOG_IF(WARNING, FLARE_UNLIKELY(expr))                           \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_PLOG_ERROR_IF(expr, ...)                             \
  PLOG_IF(ERROR, FLARE_UNLIKELY(expr))                             \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)
#define FLARE_PLOG_FATAL_IF(expr, ...)                             \
  PLOG_IF(FATAL, FLARE_UNLIKELY(expr))                             \
      << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                               __VA_ARGS__)

#ifdef _MSC_VER
#define FLARE_UNREACHABLE(...)                                              \
  do {                                                                      \
    LOG(FATAL) << "UNREACHABLE. "                                           \
               << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                        ##__VA_ARGS__);     \
  } while (0)
#define FLARE_NOT_IMPLEMENTED(...)                                          \
  do {                                                                      \
    LOG(FATAL) << "Not implemented. "                                       \
               << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                        ##__VA_ARGS__);     \
  } while (0)
#define FLARE_UNEXPECTED(...)                                               \
  do {                                                                      \
    LOG(FATAL) << "UNEXPECTED. "                                            \
               << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                        ##__VA_ARGS__);     \
  } while (0)
#else
#define FLARE_UNREACHABLE(...)                                                \
  do {                                                                        \
    [&]() __attribute__((noreturn, noinline, cold)) {                         \
      LOG(FATAL) << "UNREACHABLE. "                                           \
                 << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                          ##__VA_ARGS__);     \
      __builtin_unreachable();                                                \
    }                                                                         \
    ();                                                                       \
  } while (0)
#define FLARE_NOT_IMPLEMENTED(...)                                            \
  do {                                                                        \
    [&]() __attribute__((noreturn, noinline, cold)) {                         \
      LOG(FATAL) << "Not implemented. "                                       \
                 << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                          ##__VA_ARGS__);     \
      __builtin_unreachable();                                                \
    }                                                                         \
    ();                                                                       \
  } while (0)
#define FLARE_UNEXPECTED(...)                                                 \
  do {                                                                        \
    [&]() __attribute__((noreturn, noinline, cold)) {                         \
      LOG(FATAL) << "UNEXPECTED. "                                            \
                 << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                          ##__VA_ARGS__);     \
      __builtin_unreachable();                                                \
    }                                                                         \
    ();                                                                       \
  } while (0)
#endif

///////////////////////////////////////
// Implementation goes below.        //
///////////////////////////////////////

// C++ attribute won't work here.
//
// @sa: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89640,
#define FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD \
  __attribute__((noinline, cold))
#define FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NORETURN_NOINLINE_COLD \
  __attribute__((noreturn, noinline, cold))

namespace flare {

    namespace internal::logging {

        // Prefix writer.
        //
        // Note that the implementation MAY NOT touch whatever has been in `to`. The
        // implementation may only append its own prefix to `to`.
        //
        // Surely using `Function<...>` is superior here, but given that logging is a
        // low level facility, bring in too many dependencies is unlikely a good idea.
        using PrefixAppender = void(std::string* to);

        // Install a new logging prefix provider.
        void InstallPrefixProvider(PrefixAppender* writer);

        // Call logging prefix providers to generate the prefix for log.
        void WritePrefixTo(std::string* to);

    // FOR INTERNAL USE ONLY.
    //
    // Logging prefix providers must be registered before `main` is entered to avoid
    // potential race conditions. We provide this macro to accomplish this.
    #define FLARE_INTERNAL_LOGGING_REGISTER_PREFIX_PROVIDER(priority, cb)     \
      [[gnu::constructor(priority + 101)]] static void FLARE_CONCAT( \
          flare_reserved_logging_prefix_provider_installer_, __COUNTER__)() { \
        ::flare::internal::logging::InstallPrefixProvider(cb);                \
      }

    // Marked as noexcept. Throwing in formatting log is likely a programming
    // error.
    //
    // Forwarding-ref does not get along well with packed field, so we use
    // const-ref here.
        template <class... Ts>
        std::string format_log([[maybe_unused]] const char* file,
                              [[maybe_unused]] int line, const Ts&... args) noexcept {
            std::string result;

            WritePrefixTo(&result);
            if constexpr (sizeof...(Ts) != 0) {
                try {
                    result += fmt::format(args...);
                } catch (const std::exception& xcpt) {
                    // Presumably a wrong format string was provided?
                    LOG(FATAL) << "Failed to format log at [" << file << ":" << line
                               << "]: " << xcpt.what();
                }
            }
            return result;
        }

    }  // namespace internal::logging
}  // namespace flare

// Clang 10 has not implemented P1381R1 yet, therefore the "cold lambda" trick
// won't work quite right if structured binding identifiers are accessed during
// evaluating log message.
#if defined(__clang__) && __clang__ <= 10

#define FLARE_INTERNAL_DETAIL_LOGGING_CHECK(expr, ...)                        \
  do {                                                                        \
    if (FLARE_UNLIKELY(!(expr))) {                                            \
      ::flare::log::log_message(__FILE__, __LINE__, ::flare::LOG_FATAL).stream() \
          << "Check failed: " #expr " "                                       \
          << ::flare::internal::logging::format_log(__FILE__, __LINE__,        \
                                                   ##__VA_ARGS__);            \
      FLARE_UNREACHABLE();                                                    \
    }                                                                         \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(name, op, val1, val2, ...)      \
  do {                                                                         \
    /* `flare::log::GetReferenceableValue` triggers `-Wmaybe-uninitialized`, */    \
    /* Not sure about the reason though. */                                    \
    auto&& flare_anonymous_x = (val1);                                         \
    auto&& flare_anonymous_y = (val2);                                         \
    if (FLARE_UNLIKELY(!(flare_anonymous_x op flare_anonymous_y))) {           \
      ::flare::log::log_message_fatal(                                               \
          __FILE__, __LINE__,                                                  \
          ::flare::log::CheckOpString(::flare::log::MakeCheckOpString(                 \
              flare_anonymous_x, flare_anonymous_y, #val1 " " #op " " #val2))) \
              .stream()                                                        \
          << ::flare::internal::logging::format_log(__FILE__, __LINE__,         \
                                                   ##__VA_ARGS__);             \
      FLARE_UNREACHABLE();                                                     \
    }                                                                          \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ...)                      \
  do {                                                                       \
    if (FLARE_UNLIKELY(!(expr))) {                                           \
      ::flare::log::errno_log_message(__FILE__, __LINE__, ::flare::log::FLARE_FATAL, 0, \
                                &::flare::log_message::SendToLog)            \
              .stream()                                                      \
          << "Check failed: " #expr " "                                      \
          << ::flare::internal::logging::format_log(__FILE__, __LINE__,       \
                                                   ##__VA_ARGS__);           \
      FLARE_UNREACHABLE();                                                   \
    }                                                                        \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOG_ONCE(Level, ...)                      \
  do {                                                                  \
    static std::atomic<bool> flare_anonymous_logged{false};             \
    if (FLARE_UNLIKELY(                                                 \
            !flare_anonymous_logged.load(std::memory_order_relaxed))) { \
      if (!flare_anonymous_logged.exchange(true)) {                     \
        LOG(Level) << ::flare::internal::logging::format_log(            \
            __FILE__, __LINE__, __VA_ARGS__);                           \
      }                                                                 \
    }                                                                   \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOG_EVERY_N(Level, N, ...)                  \
  do {                                                                    \
    /* Race here, as obvious. This is how glog has done, and I haven't */ \
    /* come up with a better idea yet (unless in trade of perf.). */      \
    static int flare_anonymous_logged_counter_mod_n = 0;                  \
    if (FLARE_UNLIKELY(++flare_anonymous_logged_counter_mod_n > N)) {     \
      flare_anonymous_logged_counter_mod_n -= N;                          \
      if (flare_anonymous_logged_counter_mod_n == 1)                      \
        flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_##Level, 0,   \
                           &flare::log::log_message::SendToLog)                \
                .stream()                                                 \
            << ::flare::internal::logging::format_log(__FILE__, __LINE__,  \
                                                     __VA_ARGS__);        \
    }                                                                     \
  } while (0)

#else

#define FLARE_INTERNAL_DETAIL_LOGGING_CHECK(expr, ...)                       \
  do {                                                                       \
    if (FLARE_UNLIKELY(!(expr))) {                                           \
      /* Attribute `noreturn` is not applicable to lambda, unfortunately. */ \
      [&]() FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NORETURN_NOINLINE_COLD { \
        ::flare::log::log_message(__FILE__, __LINE__, ::flare::log::FLARE_FATAL)       \
                .stream()                                                    \
            << "Check failed: " #expr " "                                    \
            << ::flare::internal::logging::format_log(__FILE__, __LINE__,     \
                                                     ##__VA_ARGS__);         \
        FLARE_UNREACHABLE();                                                 \
      }();                                                                   \
    }                                                                        \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOGGING_CHECK_OP(name, op, val1, val2, ...)    \
  do {                                                                       \
    /* `flare::log::GetReferenceableValue` triggers `-Wmaybe-uninitialized`, */  \
    /* Not sure about the reason though. */                                  \
    auto&& flare_anonymous_x = (val1);                                       \
    auto&& flare_anonymous_y = (val2);                                       \
    if (FLARE_UNLIKELY(!(flare_anonymous_x op flare_anonymous_y))) {         \
      [&]() FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NORETURN_NOINLINE_COLD { \
        ::flare::log::log_message_fatal(                                           \
            __FILE__, __LINE__,                                              \
            ::flare::log::CheckOpString(::flare::log::MakeCheckOpString(             \
                flare_anonymous_x, flare_anonymous_y,                        \
                #val1 " " #op " " #val2)))                                   \
                .stream()                                                    \
            << ::flare::internal::logging::format_log(__FILE__, __LINE__,     \
                                                     ##__VA_ARGS__);         \
        FLARE_UNREACHABLE();                                                 \
      }();                                                                   \
    }                                                                        \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOGGING_PCHECK(expr, ...)                        \
  do {                                                                         \
    if (FLARE_UNLIKELY(!(expr))) {                                             \
      [&]() FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NORETURN_NOINLINE_COLD {   \
        ::flare::log::errno_log_message(__FILE__, __LINE__, ::flare::log::FLARE_FATAL, 0, \
                                  &::flare::log::log_message::SendToLog)            \
                .stream()                                                      \
            << "Check failed: " #expr " "                                      \
            << ::flare::internal::logging::format_log(__FILE__, __LINE__,       \
                                                     ##__VA_ARGS__);           \
        FLARE_UNREACHABLE();                                                   \
      }();                                                                     \
    }                                                                          \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOG_ONCE(Level, ...)                      \
  do {                                                                  \
    static std::atomic<bool> flare_anonymous_logged{false};             \
    if (FLARE_UNLIKELY(                                                 \
            !flare_anonymous_logged.load(std::memory_order_relaxed))) { \
      [&]() FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {     \
        if (!flare_anonymous_logged.exchange(true)) {                   \
          LOG(Level) << ::flare::internal::logging::format_log(          \
              __FILE__, __LINE__, __VA_ARGS__);                         \
        }                                                               \
      }();                                                              \
    }                                                                   \
  } while (0)

#define FLARE_INTERNAL_DETAIL_LOG_EVERY_N(Level, N, ...)                   \
  do {                                                                     \
    /* Race here, as obvious. This is how glog has done, and I haven't */  \
    /* come up with a better idea yet (unless in trade of perf.). */       \
    static int flare_anonymous_logged_counter_mod_n = 0;                   \
    if (FLARE_UNLIKELY(++flare_anonymous_logged_counter_mod_n > N)) {      \
      [&]() FLARE_INTERNAL_DETAIL_LOGGING_ATTRIBUTE_NOINLINE_COLD {        \
        flare_anonymous_logged_counter_mod_n -= N;                         \
        if (flare_anonymous_logged_counter_mod_n == 1)                     \
          flare::log::log_message(__FILE__, __LINE__, flare::log::FLARE_##Level, 0,  \
                             &flare::log::log_message::SendToLog)               \
                  .stream()                                                \
              << ::flare::internal::logging::format_log(__FILE__, __LINE__, \
                                                       __VA_ARGS__);       \
      }();                                                                 \
    }                                                                      \
  } while (0)

#endif

#define FLARE_INTERNAL_DETAIL_LOG_IF_EVERY_N(expr, Level, N, ...) \
  do {                                                            \
    if (expr) {                                                   \
      FLARE_INTERNAL_DETAIL_LOG_EVERY_N(Level, N, __VA_ARGS__);   \
    }                                                             \
  } while (0)




#endif  // FLARE_LOG_LOGGING_H_
