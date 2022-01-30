
#include "flare/base/logging.h"

#include <gflags/gflags.h>

DEFINE_bool(log_as_json, false, "Print log as a valid JSON");
DEFINE_bool(crash_on_fatal_log, false, "crash on fatal log");