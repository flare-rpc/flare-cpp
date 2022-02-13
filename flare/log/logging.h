//
// Created by liyinbin on 2022/2/13.
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
# include <unistd.h>
#include <vector>
#include <atomic>
#include <cstdint>             // the normal place uint16_t is defined
#include <gflags/gflags.h>
#include "flare/base/profile.h"
#include "flare/log/config.h"
#include "flare/log/severity.h"
#include "flare/log/vlog_is_on.h"
#include "flare/log/utility.h"
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
#define COMPACT_FLARE_LOG_TRACE flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_TRACE)
#define LOG_TO_STRING_TRACE(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_TRACE, message)
#else
#define COMPACT_FLARE_LOG_TRACE flare::log::NullStream()
#define LOG_TO_STRING_TRACE(message) flare::log::NullStream()
#endif

#if FLARE_STRIP_LOG <= 1
#define COMPACT_FLARE_LOG_DEBUG flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_DEBUG)
#define LOG_TO_STRING_DEBUG(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_DEBUG, message)
#else
#define COMPACT_FLARE_LOG_DEBUG flare::log::NullStream()
#define LOG_TO_STRING_DEBUG(message) flare::log::NullStream()
#endif

#if FLARE_STRIP_LOG <= 2
#define COMPACT_FLARE_LOG_INFO flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_INFO)
#define LOG_TO_STRING_INFO(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_INFO, message)
#else
#define COMPACT_FLARE_LOG_INFO flare::log::NullStream()
#define LOG_TO_STRING_INFO(message) flare::log::NullStream()
#endif

#if FLARE_STRIP_LOG <= 3
#define COMPACT_FLARE_LOG_WARNING flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_WARNING)
#define LOG_TO_STRING_WARNING(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_WARNING, message)
#else
#define COMPACT_FLARE_LOG_WARNING flare::log::NullStream()
#define LOG_TO_STRING_WARNING(message) flare::log::NullStream()
#endif

#if FLARE_STRIP_LOG <= 4
#define COMPACT_FLARE_LOG_ERROR flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_ERROR)
#define LOG_TO_STRING_ERROR(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_ERROR, message)
#else
#define COMPACT_FLARE_LOG_ERROR flare::log::NullStream()
#define LOG_TO_STRING_ERROR(message) flare::log::NullStream()
#endif

#if FLARE_STRIP_LOG <= 5
#define COMPACT_FLARE_LOG_FATAL flare::log::LogMessageFatal( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_FATAL(message) flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_FATAL, message)
#else
#define COMPACT_FLARE_LOG_FATAL flare::log::NullStreamFatal()
#define LOG_TO_STRING_FATAL(message) flare::log::NullStreamFatal()
#endif

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

// For DFATAL, we want to use LogMessage (as opposed to
// LogMessageFatal), to be consistent with the original behavior.
#if !DCHECK_IS_ON()
#define COMPACT_FLARE_LOG_DFATAL COMPACT_FLARE_LOG_ERROR
#elif FLARE_STRIP_LOG <= 3
#define COMPACT_FLARE_LOG_DFATAL flare::log::LogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_FATAL)
#else
#define COMPACT_FLARE_LOG_DFATAL flare::log::NullStreamFatal()
#endif

#define FLARE_LOG_TRACE(counter) flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_TRACE, counter, &flare::log::LogMessage::SendToLog)
#define SYSLOG_TRACE(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_TRACE, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_DEBUG(counter) flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_DEBUG, counter, &flare::log::LogMessage::SendToLog)
#define SYSLOG_DEBUG(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_DEBUG, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_INFO(counter) flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_INFO, counter, &flare::log::LogMessage::SendToLog)
#define SYSLOG_INFO(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_INFO, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_WARNING(counter)  \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_WARNING, counter, \
  &flare::log::LogMessage::SendToLog)
#define SYSLOG_WARNING(counter)  \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_WARNING, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_ERROR(counter)  \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_ERROR, counter, \
  &flare::log::LogMessage::SendToLog)
#define SYSLOG_ERROR(counter)  \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_ERROR, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_FATAL(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_FATAL, counter, \
  &flare::log::LogMessage::SendToLog)
#define SYSLOG_FATAL(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::FLARE_FATAL, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)
#define FLARE_LOG_DFATAL(counter) \
  flare::log::LogMessage(__FILE__, __LINE__, flare::log::DFATAL_LEVEL, counter, \
  &flare::log::LogMessage::SendToLog)
#define SYSLOG_DFATAL(counter) \
  @flare::log::LogMessage(__FILE__, __LINE__, flare::log::DFATAL_LEVEL, counter, \
  &flare::log::LogMessage::SendToSyslogAndLog)

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token FLARE_LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.
#define LOG(severity) COMPACT_FLARE_LOG_ ## severity.stream()
#define SYSLOG(severity) SYSLOG_ ## severity(0).stream()

namespace flare::log {

    // They need the definitions of integer types.

    // Initialize google's logging library. You will see the program name
    // specified by argv0 in log outputs.
    FLARE_EXPORT void InitGoogleLogging(const char *argv0);

    // Shutdown google's logging library.
    FLARE_EXPORT void ShutdownGoogleLogging();

    // Install a function which will be called after LOG(FATAL).
    FLARE_EXPORT void InstallFailureFunction(void (*fail_func)());

    // Enable/Disable old log cleaner.
    FLARE_EXPORT void EnableLogCleaner(int overdue_days);

    FLARE_EXPORT void DisableLogCleaner();

    FLARE_EXPORT void SetApplicationFingerprint(const std::string &fingerprint);

    class LogSink;  // defined below

    // If a non-NULL sink pointer is given, we push this message to that sink.
    // For LOG_TO_SINK we then do normal LOG(severity) logging as well.
    // This is useful for capturing messages and passing/storing them
    // somewhere more specific than the global log of the process.
    // Argument types:
    //   LogSink* sink;
    //   log_severity severity;
    // The cast is to disambiguate NULL arguments.
#define LOG_TO_SINK(sink, severity) \
  flare::log::LogMessage(                                    \
      __FILE__, __LINE__,                                               \
      flare::log::FLARE_ ## severity,                         \
      static_cast<flare::log::LogSink*>(sink), true).stream()
#define LOG_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                  \
  flare::log::LogMessage(                                    \
      __FILE__, __LINE__,                                               \
      flare::log::FLARE_ ## severity,                         \
      static_cast<flare::log::LogSink*>(sink), false).stream()

// If a non-NULL string pointer is given, we write this message to that string.
// We then do normal LOG(severity) logging as well.
// This is useful for capturing messages and storing them somewhere more
// specific than the global log of the process.
// Argument types:
//   string* message;
//   log_severity severity;
// The cast is to disambiguate NULL arguments.
// NOTE: LOG(severity) expands to LogMessage().stream() for the specified
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
  !(condition) ? (void) 0 : flare::log::LogMessageVoidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) \
  static_cast<void>(0),                \
  !(condition) ? (void) 0 : flare::log::LogMessageVoidify() & SYSLOG(severity)

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
    inline void MakeCheckOpValueString(std::ostream *os, const T &v) {
        (*os) << v;
    }

    // Overrides for char types provide readable values for unprintable
    // characters.
    template<>
    FLARE_EXPORT
    void MakeCheckOpValueString(std::ostream *os, const char &v);

    template<>
    FLARE_EXPORT
    void MakeCheckOpValueString(std::ostream *os, const signed char &v);

    template<>
    FLARE_EXPORT
    void MakeCheckOpValueString(std::ostream *os, const unsigned char &v);

// This is required because nullptr is only present in c++ 11 and later.
// Provide printable value for nullptr_t
    template<>
    FLARE_EXPORT
    void MakeCheckOpValueString(std::ostream *os, const std::nullptr_t &v);

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
        MakeCheckOpValueString(comb.ForVar1(), v1);
        MakeCheckOpValueString(comb.ForVar2(), v2);
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
#define CHECK_OP_LOG(name, op, val1, val2, log)                         \
  while (flare::log::_Check_string* _result =                \
        flare::log::Check##name##Impl(                      \
             flare::log::GetReferenceableValue(val1),        \
             flare::log::GetReferenceableValue(val2),        \
#val1 " " #op " " #val2))                                  \
    log(__FILE__, __LINE__,                                             \
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
  CHECK_OP_LOG(name, op, val1, val2, flare::log::LogMessageFatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, flare::log::NullStreamFatal)
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
  flare::log::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

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
  flare::log::ErrnoLogMessage( \
      __FILE__, __LINE__, flare::log::FLARE_ ## severity, counter, \
      &flare::log::LogMessage::SendToLog)

#define PLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  !(condition) ? (void) 0 : flare::log::LogMessageVoidify() & PLOG(severity)

// A CHECK() macro that postpends errno if the condition is false. E.g.
//
// if (poll(fds, nfds, timeout) == -1) { PCHECK(errno == EINTR); ... }
#define PCHECK(condition)  \
      PLOG_IF(FATAL, GOOGLE_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
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
    } // namespace google

    // We need to identify the static variables as "benign" races
    // to avoid noisy reports from TSAN.
    extern "C" void AnnotateBenignRaceSized(
      const char *file,
      int line,
      const volatile void *mem,
      long size,
      const char *description);

    namespace google {
#endif

#define SOME_KIND_OF_LOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    flare::log::LogMessage( \
        __FILE__, __LINE__, flare::log::FLARE_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (condition && \
      ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
    flare::log::LogMessage( \
        __FILE__, __LINE__, flare::log::GLOG_ ## severity, LOG_OCCURRENCES, \
                 &what_to_do).stream()

#define SOME_KIND_OF_PLOG_EVERY_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0), LOG_OCCURRENCES_MOD_N(0); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES_MOD_N, sizeof(int), "")); \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    flare::log::ErrnoLogMessage( \
        __FILE__, __LINE__, flare::log::GLOG_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_FIRST_N(severity, n, what_to_do) \
  static std::atomic<int> LOG_OCCURRENCES(0); \
  _GLOG_IFDEF_THREAD_SANITIZER(AnnotateBenignRaceSized(__FILE__, __LINE__, &LOG_OCCURRENCES, sizeof(int), "")); \
  if (LOG_OCCURRENCES <= n) \
    ++LOG_OCCURRENCES; \
  if (LOG_OCCURRENCES <= n) \
    flare::log::LogMessage( \
        __FILE__, __LINE__, flare::log::GLOG_ ## severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

    namespace log_internal {
        template<bool>
        struct CompileAssert {
        };
        struct CrashReason;

// Returns true if FailureSignalHandler is installed.
// Needs to be exported since it's used by the signalhandler_unittest.
        FLARE_EXPORT bool IsFailureSignalHandlerInstalled();
    }  // namespace log_internal

#define LOG_EVERY_N(severity, n)                                        \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), flare::log::LogMessage::SendToLog)

#define SYSLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), flare::log::LogMessage::SendToSyslogAndLog)

#define PLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_PLOG_EVERY_N(severity, (n), flare::log::LogMessage::SendToLog)

#define LOG_FIRST_N(severity, n) \
  SOME_KIND_OF_LOG_FIRST_N(severity, (n), flare::log::LogMessage::SendToLog)

#define LOG_IF_EVERY_N(severity, condition, n) \
  SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n), flare::log::LogMessage::SendToLog)

// We want the special COUNTER value available for LOG_EVERY_X()'ed messages
    enum PRIVATE_Counter {
        COUNTER
    };

#ifdef GLOG_NO_ABBREVIATED_SEVERITIES
    // wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
    // substituted with 0, and it expands to COMPACT_FLARE_LOG_0. To allow us
    // to keep using this syntax, we define this macro to do the same thing
    // as COMPACT_FLARE_LOG_ERROR.
#define COMPACT_FLARE_LOG_0 COMPACT_FLARE_LOG_ERROR
#define SYSLOG_0 SYSLOG_ERROR
#define LOG_TO_STRING_0 LOG_TO_STRING_ERROR
    // Needed for LOG_IS_ON(ERROR).
    const log_severity GLOG_0 = FLARE_ERROR;
#else
// Users may include windows.h after logging.h without
// GLOG_NO_ABBREVIATED_SEVERITIES nor WIN32_LEAN_AND_MEAN.
// For this case, we cannot detect if ERROR is defined before users
// actually use ERROR. Let's make an undefined symbol to warn users.
# define GLOG_ERROR_MSG ERROR_macro_is_defined_Define_GLOG_NO_ABBREVIATED_SEVERITIES_before_including_logging_h_See_the_document_for_detail
# define COMPACT_FLARE_LOG_0 GLOG_ERROR_MSG
# define SYSLOG_0 GLOG_ERROR_MSG
# define LOG_TO_STRING_0 GLOG_ERROR_MSG
# define GLOG_0 GLOG_ERROR_MSG
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
  true ? (void) 0 : flare::log::LogMessageVoidify() & LOG(severity)

#define DVLOG(verboselevel)             \
  static_cast<void>(0),                 \
  (true || !VLOG_IS_ON(verboselevel)) ? \
      (void) 0 : flare::log::LogMessageVoidify() & LOG(INFO)

#define DLOG_IF(severity, condition) \
  static_cast<void>(0),              \
  (true || !(condition)) ? (void) 0 : flare::log::LogMessageVoidify() & LOG(severity)

#define DLOG_EVERY_N(severity, n) \
  static_cast<void>(0),           \
  true ? (void) 0 : flare::log::LogMessageVoidify() & LOG(severity)

#define DLOG_IF_EVERY_N(severity, condition, n) \
  static_cast<void>(0),                         \
  (true || !(condition))? (void) 0 : flare::log::LogMessageVoidify() & LOG(severity)

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

// LogMessage::LogStream is a std::ostream backed by this streambuf.
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
// create an instance of LogMessage and then stream stuff to it.
// When you finish streaming to it, ~LogMessage is called and the
// full message gets streamed to the appropriate destination.
//
// You shouldn't actually use LogMessage's constructor to log things,
// though.  You should use the LOG() macro (and variants thereof)
// above.
    class FLARE_EXPORT LogMessage {
    public:
        enum {
            // Passing kNoLogPrefix for the line number disables the
            // log-message prefix. Useful for using the LogMessage
            // infrastructure as a printing utility. See also the --log_prefix
            // flag for controlling the log-message prefix on an
            // application-wide basis.
            kNoLogPrefix = -1
        };

        // LogStream inherit from non-DLL-exported class (std::ostrstream)
        // and VC++ produces a warning for this situation.
        // However, MSDN says "C4275 can be ignored in Microsoft Visual C++
        // 2005 if you are deriving from a type in the Standard C++ Library"
        // http://msdn.microsoft.com/en-us/library/3tdb471s(VS.80).aspx
        // Let's just ignore the warning.
        class FLARE_EXPORT LogStream : public std::ostream {
        public:
            LogStream(char *buf, int len, uint64_t ctr)
                    : std::ostream(NULL),
                      streambuf_(buf, len),
                      ctr_(ctr),
                      self_(this) {
                rdbuf(&streambuf_);
            }

            uint64_t ctr() const { return ctr_; }

            void set_ctr(uint64_t ctr) { ctr_ = ctr; }

            LogStream *self() const { return self_; }

            // Legacy std::streambuf methods.
            size_t pcount() const { return streambuf_.pcount(); }

            char *pbase() const { return streambuf_.pbase(); }

            char *str() const { return pbase(); }

        private:
            LogStream(const LogStream &);

            LogStream &operator=(const LogStream &);

            base_logging::LogStreamBuf streambuf_;
            uint64_t ctr_;  // Counter hack (for the LOG_EVERY_X() macro)
            LogStream *self_;  // Consistency check hack
        };

    public:
        // icc 8 requires this typedef to avoid an internal compiler error.
        typedef void (LogMessage::*SendMethod)();

        LogMessage(const char *file, int line, log_severity severity, uint64_t ctr,
                   SendMethod send_method);

        // Two special constructors that generate reduced amounts of code at
        // LOG call sites for common cases.

        // Used for LOG(INFO): Implied are:
        // severity = INFO, ctr = 0, send_method = &LogMessage::SendToLog.
        //
        // Using this constructor instead of the more complex constructor above
        // saves 19 bytes per call site.
        LogMessage(const char *file, int line);

        // Used for LOG(severity) where severity != INFO.  Implied
        // are: ctr = 0, send_method = &LogMessage::SendToLog
        //
        // Using this constructor instead of the more complex constructor above
        // saves 17 bytes per call site.
        LogMessage(const char *file, int line, log_severity severity);

        // Constructor to log this message to a specified sink (if not NULL).
        // Implied are: ctr = 0, send_method = &LogMessage::SendToSinkAndLog if
        // also_send_to_log is true, send_method = &LogMessage::SendToSink otherwise.
        LogMessage(const char *file, int line, log_severity severity, LogSink *sink,
                   bool also_send_to_log);

        // Constructor where we also give a vector<string> pointer
        // for storing the messages (if the pointer is not NULL).
        // Implied are: ctr = 0, send_method = &LogMessage::SaveOrSendToLog.
        LogMessage(const char *file, int line, log_severity severity,
                   std::vector<std::string> *outvec);

        // Constructor where we also give a string pointer for storing the
        // message (if the pointer is not NULL).  Implied are: ctr = 0,
        // send_method = &LogMessage::WriteToStringAndLog.
        LogMessage(const char *file, int line, log_severity severity,
                   std::string *message);

        // A special constructor used for check failures
        LogMessage(const char *file, int line, const CheckOpString &result);

        ~LogMessage();

        // Flush a buffered message to the sink set in the constructor.  Always
        // called by the destructor, it may also be called from elsewhere if
        // needed.  Only the first call is actioned; any later ones are ignored.
        void Flush();

        // An arbitrary limit on the length of a single log message.  This
        // is so that streaming can be done more efficiently.
        static const size_t kMaxLogMessageLen;

        // Theses should not be called directly outside of logging.*,
        // only passed as SendMethod arguments to other LogMessage methods:
        void SendToLog();  // Actually dispatch to the logs
        void SendToSyslogAndLog();  // Actually dispatch to syslog and the logs

        // Call abort() or similar to perform LOG(FATAL) crash.
        static void Fail();

        std::ostream &stream();

        int preserved_errno() const;

        // Must be called without the log_mutex held.  (L < log_mutex)
        static int64_t num_messages(int severity);

        struct LogMessageData;

    private:
        // Fully internal SendMethod cases:
        void SendToSinkAndLog();  // Send to sink if provided and dispatch to the logs
        void SendToSink();  // Send to sink if provided, do nothing otherwise.

        // Write to string if provided and dispatch to the logs.
        void WriteToStringAndLog();

        void SaveOrSendToLog();  // Save to stringvec if provided, else to logs

        void Init(const char *file, int line, log_severity severity,
                  void (LogMessage::*send_method)());

        // Used to fill in crash information during LOG(FATAL) failures.
        void RecordCrashReason(log_internal::CrashReason *reason);

        // Counts of messages sent at each priority:
        static int64_t num_messages_[NUM_SEVERITIES];  // under log_mutex

        // We keep the data in a separate struct so that each instance of
        // LogMessage uses less stack space.
        LogMessageData *allocated_;
        LogMessageData *data_;

        friend class LogDestination;

        LogMessage(const LogMessage &);

        void operator=(const LogMessage &);
    };

    // This class happens to be thread-hostile because all instances share
    // a single data buffer, but since it can only be created just before
    // the process dies, we don't worry so much.
    class FLARE_EXPORT LogMessageFatal : public LogMessage {
    public:
        LogMessageFatal(const char *file, int line);

        LogMessageFatal(const char *file, int line, const CheckOpString &result);

        ~LogMessageFatal();
    };

    // A non-macro interface to the log facility; (useful
    // when the logging level is not a compile-time constant).
    inline void LogAtLevel(int const severity, std::string const &msg) {
        LogMessage(__FILE__, __LINE__, severity).stream() << msg;
    }

// A macro alternative of LogAtLevel. New code may want to use this
// version since there are two advantages: 1. this version outputs the
// file name and the line number where this macro is put like other
// LOG macros, 2. this macro can be used as C++ stream.
#define LOG_AT_LEVEL(severity) flare::log::LogMessage(__FILE__, __LINE__, severity).stream()

// Check if it's compiled in C++11 mode.
//
// GXX_EXPERIMENTAL_CXX0X is defined by gcc and clang up to at least
// gcc-4.7 and clang-3.1 (2011-12-13).  __cplusplus was defined to 1
// in gcc before 4.7 (Crosstool 16) and clang before 3.1, but is
// defined according to the language version in effect thereafter.
// Microsoft Visual Studio 14 (2015) sets __cplusplus==199711 despite
// reasonably good C++11 support, so we set LANG_CXX for it and
// newer versions (_MSC_VER >= 1900).
#if (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L || \
     (defined(_MSC_VER) && _MSC_VER >= 1900)) && !defined(__UCLIBCXX_MAJOR__)

    // Helper for CHECK_NOTNULL().
    //
    // In C++11, all cases can be handled by a single function. Since the value
    // category of the argument is preserved (also for rvalue references),
    // member initializer lists like the one below will compile correctly:
    //
    //   Foo()
    //     : x_(CHECK_NOTNULL(MethodReturningUniquePtr())) {}
    template<typename T>
    T CheckNotNull(const char *file, int line, const char *names, T &&t) {
        if (t == nullptr) {
            LogMessageFatal(file, line, new std::string(names));
        }
        return std::forward<T>(t);
    }

#else

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T* CheckNotNull(const char *file, int line, const char *names, T* t) {
      if (t == NULL) {
        LogMessageFatal(file, line, new std::string(names));
      }
      return t;
    }
#endif

    // Allow folks to put a counter in the LOG_EVERY_X()'ed messages. This
    // only works if ostream is a LogStream. If the ostream is not a
    // LogStream you'll get an assert saying as much at runtime.
    FLARE_EXPORT std::ostream &operator<<(std::ostream &os,
                                          const PRIVATE_Counter &);


    // Derived class for PLOG*() above.
    class FLARE_EXPORT ErrnoLogMessage : public LogMessage {
    public:

        ErrnoLogMessage(const char *file, int line, log_severity severity, uint64_t ctr,
                        void (LogMessage::*send_method)());

        // Postpends ": strerror(errno) [errno]".
        ~ErrnoLogMessage();

    private:
        ErrnoLogMessage(const ErrnoLogMessage &);

        void operator=(const ErrnoLogMessage &);
    };


    // This class is used to explicitly ignore values in the conditional
    // logging macros.  This avoids compiler warnings like "value computed
    // is not used" and "statement has no effect".

    class FLARE_EXPORT LogMessageVoidify {
    public:
        LogMessageVoidify() {}

        // This has to be an operator with a precedence lower than << but
        // higher than ?:
        void operator&(std::ostream &) {}
    };


    // Flushes all log files that contains messages that are at least of
    // the specified severity level.  Thread-safe.
    FLARE_EXPORT void FlushLogFiles(log_severity min_severity);

    // Flushes all log files that contains messages that are at least of
    // the specified severity level. Thread-hostile because it ignores
    // locking -- used for catastrophic failures.
    FLARE_EXPORT void FlushLogFilesUnsafe(log_severity min_severity);

    //
    // Set the destination to which a particular severity level of log
    // messages is sent.  If base_filename is "", it means "don't log this
    // severity".  Thread-safe.
    //
    FLARE_EXPORT void SetLogDestination(log_severity severity,
                                        const char *base_filename);

    //
    // Set the basename of the symlink to the latest log file at a given
    // severity.  If symlink_basename is empty, do not make a symlink.  If
    // you don't call this function, the symlink basename is the
    // invocation name of the program.  Thread-safe.
    //
    FLARE_EXPORT void SetLogSymlink(log_severity severity,
                                    const char *symlink_basename);

    //
    // Used to send logs to some other kind of destination
    // Users should subclass LogSink and override send to do whatever they want.
    // Implementations must be thread-safe because a shared instance will
    // be called from whichever thread ran the LOG(XXX) line.
    class FLARE_EXPORT LogSink {
    public:
        virtual ~LogSink();

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
        // but before that LogMessage exits or crashes.
        // By default this function does nothing.
        // Using this function one can implement complex logic for send()
        // that itself involves logging; and do all this w/o causing deadlocks and
        // inconsistent rearrangement of log messages.
        // E.g. if a LogSink has thread-specific actions, the send() method
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

    // Add or remove a LogSink as a consumer of logging data.  Thread-safe.
    FLARE_EXPORT void AddLogSink(LogSink *destination);

    FLARE_EXPORT void RemoveLogSink(LogSink *destination);

    //
    // Specify an "extension" added to the filename specified via
    // SetLogDestination.  This applies to all severity levels.  It's
    // often used to append the port we're listening on to the logfile
    // name.  Thread-safe.
    //
    FLARE_EXPORT void SetLogFilenameExtension(
            const char *filename_extension);

    //
    // Make it so that all log messages of at least a particular severity
    // are logged to stderr (in addition to logging to the usual log
    // file(s)).  Thread-safe.
    //
    FLARE_EXPORT void SetStderrLogging(log_severity min_severity);

    //
    // Make it so that all log messages go only to stderr.  Thread-safe.
    //
    FLARE_EXPORT void LogToStderr();

    //
    // Make it so that all log messages of at least a particular severity are
    // logged via email to a list of addresses (in addition to logging to the
    // usual log file(s)).  The list of addresses is just a string containing
    // the email addresses to send to (separated by spaces, say).  Thread-safe.
    //
    FLARE_EXPORT void SetEmailLogging(log_severity min_severity,
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
    FLARE_EXPORT void GetExistingTempDirectories(
            std::vector<std::string> *list);

// Print any fatal message again -- useful to call from signal handler
// so that the last thing in the output is the fatal message.
// Thread-hostile, but a race is unlikely.
    FLARE_EXPORT void ReprintFatalMessage();

    // Truncate a log file that may be the append-only output of multiple
    // processes and hence can't simply be renamed/reopened (typically a
    // stdout/stderr).  If the file "path" is > "limit" bytes, copy the
    // last "keep" bytes to offset 0 and truncate the rest. Since we could
    // be racing with other writers, this approach has the potential to
    // lose very small amounts of data. For security, only follow symlinks
    // if the path is /proc/self/fd/*
    FLARE_EXPORT void TruncateLogFile(const char *path,
                                      int64_t limit, int64_t keep);

    // Truncate stdout and stderr if they are over the value specified by
    // --max_log_size; keep the final 1MB.  This function has the same
    // race condition as TruncateLogFile.
    FLARE_EXPORT void TruncateStdoutStderr();

    // Return the string representation of the provided log_severity level.
    // Thread-safe.
    FLARE_EXPORT const char *GetLogSeverityName(log_severity severity);

    // ---------------------------------------------------------------------
    // Implementation details that are not useful to most clients
    // ---------------------------------------------------------------------

    // A Logger is the interface used by logging modules to emit entries
    // to a log.  A typical implementation will dump formatted data to a
    // sequence of files.  We also provide interfaces that will forward
    // the data to another thread so that the invoker never blocks.
    // Implementations should be thread-safe since the logging system
    // will write to them from multiple threads.

    namespace base {

        class FLARE_EXPORT Logger {
        public:
            virtual ~Logger();

            // Writes "message[0,message_len-1]" corresponding to an event that
            // occurred at "timestamp".  If "force_flush" is true, the log file
            // is flushed immediately.
            //
            // The input message has already been formatted as deemed
            // appropriate by the higher level logging facility.  For example,
            // textual log messages already contain timestamps, and the
            // file:linenumber header.
            virtual void Write(bool force_flush,
                               time_t timestamp,
                               const char *message,
                               int message_len) = 0;

            // Flush any buffered messages
            virtual void Flush() = 0;

            // Get the current LOG file size.
            // The returned value is approximate since some
            // logged data may not have been flushed to disk yet.
            virtual uint32_t LogSize() = 0;
        };

        // Get the logger for the specified severity level.  The logger
        // remains the property of the logging module and should not be
        // deleted by the caller.  Thread-safe.
        extern FLARE_EXPORT Logger *GetLogger(log_severity level);

        // Set the logger for the specified severity level.  The logger
        // becomes the property of the logging module and should not
        // be deleted by the caller.  Thread-safe.
        extern FLARE_EXPORT void SetLogger(log_severity level, Logger *logger);

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
    class FLARE_EXPORT NullStream : public LogMessage::LogStream {
    public:
        // Initialize the LogStream so the messages can be written somewhere
        // (they'll never be actually displayed). This will be needed if a
        // NullStream& is implicitly converted to LogStream&, in which case
        // the overloaded NullStream::operator<< will not be invoked.
        NullStream() : LogMessage::LogStream(message_buffer_, 1, 0) {}

        NullStream(const char * /*file*/, int /*line*/,
                   const CheckOpString & /*result*/) :
                LogMessage::LogStream(message_buffer_, 1, 0) {}

        NullStream &stream() { return *this; }

    private:
        // A very short buffer for messages (which we discard anyway). This
        // will be needed if NullStream& converted to LogStream& (e.g. as a
        // result of a conditional expression).
        char message_buffer_[2];
    };

    // Do nothing. This operator is inline, allowing the message to be
    // compiled away. The message will not be compiled away if we do
    // something like (flag ? LOG(INFO) : LOG(ERROR)) << message; when
    // SKIP_LOG=WARNING. In those cases, NullStream will be implicitly
    // converted to LogStream and the message will be computed and then
    // quietly discarded.
    template<class T>
    inline NullStream &operator<<(NullStream &str, const T &) { return str; }

    // Similar to NullStream, but aborts the program (without stack
    // trace), like LogMessageFatal.
    class FLARE_EXPORT NullStreamFatal : public NullStream {
    public:
        NullStreamFatal() {}

        NullStreamFatal(const char *file, int line, const CheckOpString &result) :
                NullStream(file, line, result) {}

        ~NullStreamFatal() throw() { _exit(1); }
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


#endif  // FLARE_LOG_LOGGING_H_
