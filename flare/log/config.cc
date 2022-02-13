//
// Created by jeff.li.
//

#include "flare/log/config.h"
#include "flare/log/severity.h"
#include <gflags/gflags.h>

static bool BoolFromEnv(const char *varname, bool defval) {
    const char *const valstr = getenv(varname);
    if (!valstr) {
        return defval;
    }
    return memchr("tTyY1\0", valstr[0], 6) != NULL;
}

FLARE_LOG_DEFINE_bool(timestamp_in_logfile_name,
                      BoolFromEnv("GOOGLE_TIMESTAMP_IN_LOGFILE_NAME", true),
                      "put a timestamp at the end of the log file name");
FLARE_LOG_DEFINE_bool(logtostderr, BoolFromEnv("GOOGLE_LOGTOSTDERR", false),
                      "log messages go to stderr instead of logfiles");
FLARE_LOG_DEFINE_bool(alsologtostderr, BoolFromEnv("GOOGLE_ALSOLOGTOSTDERR", false),
                      "log messages go to stderr in addition to logfiles");
FLARE_LOG_DEFINE_bool(colorlogtostderr, false,
                      "color messages logged to stderr (if supported by terminal)");
#ifdef FLARE_PLATFORM_LINUX
FLARE_LOG_DEFINE_bool(drop_log_memory, true, "Drop in-memory buffers of log contents. "
                 "Logs can grow very quickly and they are rarely read before they "
                 "need to be evicted from memory. Instead, drop them from memory "
                 "as soon as they are flushed to disk.");
#endif

// By default, errors (including fatal errors) get logged to stderr as
// well as the file.
//
// The default is ERROR instead of FATAL so that users can see problems
// when they run a program without having to look in another file.
DEFINE_int32(stderrthreshold,
             flare::log::FLARE_ERROR,
             "log messages at or above this level are copied to stderr in "
             "addition to logfiles.  This flag obsoletes --alsologtostderr.");

FLARE_LOG_DEFINE_string(alsologtoemail, "",
                        "log messages go to these email addresses "
                        "in addition to logfiles");
FLARE_LOG_DEFINE_bool(log_prefix, true,
                      "Prepend the log prefix to the start of each log line");
FLARE_LOG_DEFINE_int32(minloglevel, 2, "Messages logged at a lower level than this don't "
                                       "actually get logged anywhere");
FLARE_LOG_DEFINE_int32(logbuflevel, 2,
                       "Buffer log messages logged at this level or lower"
                       " (-1 means don't buffer; 0 means buffer INFO only;"
                       " ...)");
FLARE_LOG_DEFINE_int32(logbufsecs, 30,
                       "Buffer log messages for at most this many seconds");
FLARE_LOG_DEFINE_int32(logemaillevel, 999,
                       "Email log messages logged at this level or higher"
                       " (0 means email all; 3 means email FATAL only;"
                       " ...)");
FLARE_LOG_DEFINE_string(logmailer, "",
                        "Mailer used to send logging email");


// Compute the default value for --log_dir
static const char *DefaultLogDir() {
    const char *env;
    env = getenv("GOOGLE_LOG_DIR");
    if (env != NULL && env[0] != '\0') {
        return env;
    }
    env = getenv("TEST_TMPDIR");
    if (env != NULL && env[0] != '\0') {
        return env;
    }
    return "";
}

FLARE_LOG_DEFINE_int32(logfile_mode, 0664, "Log file mode/permissions.");

FLARE_LOG_DEFINE_string(log_dir, DefaultLogDir(),
                        "If specified, logfiles are written into this directory instead "
                        "of the default logging directory.");
FLARE_LOG_DEFINE_string(log_link, "", "Put additional links to the log "
                                      "files in this directory");

FLARE_LOG_DEFINE_int32(max_log_size, 1800,
                       "approx. maximum log file size (in MB). A value of 0 will "
                       "be silently overridden to 1.");

FLARE_LOG_DEFINE_bool(stop_logging_if_full_disk, false,
                      "Stop attempting to log to disk if the disk is full.");

FLARE_LOG_DEFINE_string(log_backtrace_at, "",
                        "Emit a backtrace when logging at file:linenum.");

FLARE_LOG_DEFINE_bool(log_utc_time, false,
                      "Use UTC time for logging.");

DEFINE_bool(log_as_json, false, "Print log as a valid JSON");
DEFINE_bool(crash_on_fatal_log, false, "crash on fatal log");
