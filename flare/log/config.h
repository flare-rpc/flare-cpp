//
// Created by jeff.li.
//

#ifndef FLARE_LOG_CONFIG_H_
#define FLARE_LOG_CONFIG_H_

#include <cstdlib>               // for getenv
#include <cstring>               // for memchr
#include <string>


#include <gflags/gflags.h>

// Define FLARE_LOG_DEFINE_* using DEFINE_* . By using these macros, we
// have FLARE_LOG_* environ variables even if we have gflags installed.
//
// If both an environment variable and a flag are specified, the value
// specified by a flag wins. E.g., if FLARE_LOG_v=0 and --v=1, the
// verbosity will be 1, not 0.

#define FLARE_LOG_DEFINE_bool(name, value, meaning) \
  DEFINE_bool(name, ENV_TO_BOOL("FLARE_LOG_" #name, value), meaning)

#define FLARE_LOG_DEFINE_int32(name, value, meaning) \
  DEFINE_int32(name, ENV_TO_INT("FLARE_LOG_" #name, value), meaning)

#define FLARE_LOG_DEFINE_string(name, value, meaning) \
  DEFINE_string(name, ENV_TO_STRING("FLARE_LOG_" #name, value), meaning)

// These macros (could be functions, but I don't want to bother with a .cc
// file), make it easier to initialize flags from the environment.

#define ENV_TO_STRING(envname, dflt)   \
  (!getenv(envname) ? (dflt) : getenv(envname))

#define ENV_TO_BOOL(envname, dflt)   \
  (!getenv(envname) ? (dflt) : memchr("tTyY1\0", getenv(envname)[0], 6) != NULL)

#define ENV_TO_INT(envname, dflt)  \
  (!getenv(envname) ? (dflt) : strtol(getenv(envname), NULL, 10))



// Set whether appending a timestamp to the log file name
DECLARE_bool(timestamp_in_logfile_name);

// Set whether log messages go to stderr instead of logfiles
DECLARE_bool(logtostderr);

// Set whether log messages go to stderr in addition to logfiles.
DECLARE_bool(alsologtostderr);

// Set color messages logged to stderr (if supported by terminal).
DECLARE_bool(colorlogtostderr);

// Log messages at a level >= this flag are automatically sent to
// stderr in addition to log files.
DECLARE_int32(stderrthreshold);

// Set whether the log prefix should be prepended to each line of output.
DECLARE_bool(log_prefix);

// Log messages at a level <= this flag are buffered.
// Log messages at a higher level are flushed immediately.
DECLARE_int32(logbuflevel);

// Sets the maximum number of seconds which logs may be buffered for.
DECLARE_int32(logbufsecs);

// Log suppression level: messages logged at a lower level than this
// are suppressed.
DECLARE_int32(minloglevel);

// If specified, logfiles are written into this directory instead of the
// default logging directory.
DECLARE_string(log_dir);

// Set the log file mode.
DECLARE_int32(logfile_mode);

// days to save log
DECLARE_int32(log_save_days);

DECLARE_int32(logemaillevel);

// Sets the path of the directory into which to put additional links
// to the log files.
DECLARE_string(log_link);

DECLARE_string(alsologtoemail);

DECLARE_string(logmailer);

DECLARE_string(log_backtrace_at);

DECLARE_int32(v);  // in vlog_is_on.cc

DECLARE_string(vmodule); // also in vlog_is_on.cc
// Sets the maximum log file size (in MB).
DECLARE_int32(max_log_size);

// Sets whether to avoid logging to the disk if the disk is full.
DECLARE_bool(stop_logging_if_full_disk);

// Use UTC time for logging
DECLARE_bool(log_utc_time);

DECLARE_bool(crash_on_fatal_log);

DECLARE_bool(log_as_json);

#define HAVE_STACKTRACE
#define HAVE_SIGACTION
#define HAVE_SYMBOLIZE

#endif  // FLARE_LOG_CONFIG_H_
