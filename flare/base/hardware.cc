
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/base/hardware.h"
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)

#include <sys/sysctl.h>

#endif
#include <string.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <mutex>
#include <thread>  // NOLINT(build/c++11)
#include <utility>
#include <vector>

#include "flare/log/logging.h"
#include "flare/times/internal/unscaled_cycle_clock.h"

namespace flare {

    static int g_num_cpus = 0;
    static double g_nominal_cpu_frequency = 1.0;

    static void initialize_system_info();
    static double get_nominal_cpu_frequency();
    std::once_flag g_sysinfo_flag;

    double nominal_cpu_frequency() {
        std::call_once(g_sysinfo_flag, initialize_system_info);
        return g_nominal_cpu_frequency;
    }

    int num_cpus() {
        std::call_once(g_sysinfo_flag, initialize_system_info);
        return g_num_cpus;
    }

    void initialize_system_info() {
        g_nominal_cpu_frequency = get_nominal_cpu_frequency();
        g_num_cpus = std::thread::hardware_concurrency();
    }

#if defined(CTL_HW) && defined(HW_CPU_FREQ)

    double get_nominal_cpu_frequency() {
        unsigned freq;
        size_t size = sizeof(freq);
        int mib[2] = {CTL_HW, HW_CPU_FREQ};
        if (sysctl(mib, 2, &freq, &size, nullptr, 0) == 0) {
            return static_cast<double>(freq);
        }
        return 1.0;
    }

#else

    // Helper function for reading a long from a file. Returns true if successful
    // and the memory location pointed to by value is set to the value read.
    static bool read_long_from_file(const char *file, long *value) {
        bool ret = false;
        int fd = open(file, O_RDONLY);
        if (fd != -1) {
            char line[1024];
            char *err;
            memset(line, '\0', sizeof(line));
            int len = read(fd, line, sizeof(line) - 1);
            if (len <= 0) {
                ret = false;
            } else {
                const long temp_value = strtol(line, &err, 10);
                if (line[0] != '\0' && (*err == '\n' || *err == '\0')) {
                    *value = temp_value;
                    ret = true;
                }
            }
            close(fd);
        }
        return ret;
    }

#if defined(FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY)

    // Reads a monotonic time source and returns a value in
// nanoseconds. The returned value uses an arbitrary epoch, not the
// Unix epoch.
static int64_t read_monotonic_clock_nanos() {
  struct timespec t;
#ifdef CLOCK_MONOTONIC_RAW
  int rc = clock_gettime(CLOCK_MONOTONIC_RAW, &t);
#else
  int rc = clock_gettime(CLOCK_MONOTONIC, &t);
#endif
  if (rc != 0) {
    perror("clock_gettime() failed");
    abort();
  }
  return int64_t{t.tv_sec} * 1000000000 + t.tv_nsec;
}

class unscaled_cycle_clock_wrapper_for_initialize_frequency {
 public:
  static int64_t now() { return times_internal::unscaled_cycle_clock::now(); }
};

struct TimeTscPair {
  int64_t time;  // From read_monotonic_clock_nanos().
  int64_t tsc;   // From unscaled_cycle_clock::now().
};

// Returns a pair of values (monotonic kernel time, TSC ticks) that
// approximately correspond to each other.  This is accomplished by
// doing several reads and picking the reading with the lowest
// latency.  This approach is used to minimize the probability that
// our thread was preempted between clock reads.
static TimeTscPair GetTimeTscPair() {
  int64_t best_latency = std::numeric_limits<int64_t>::max();
  TimeTscPair best;
  for (int i = 0; i < 10; ++i) {
    int64_t t0 = read_monotonic_clock_nanos();
    int64_t tsc = unscaled_cycle_clock_wrapper_for_initialize_frequency::now();
    int64_t t1 = read_monotonic_clock_nanos();
    int64_t latency = t1 - t0;
    if (latency < best_latency) {
      best_latency = latency;
      best.time = t0;
      best.tsc = tsc;
    }
  }
  return best;
}

// Measures and returns the TSC frequency by taking a pair of
// measurements approximately `sleep_nanoseconds` apart.
static double MeasureTscFrequencyWithSleep(int sleep_nanoseconds) {
  auto t0 = GetTimeTscPair();
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = sleep_nanoseconds;
  while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {}
  auto t1 = GetTimeTscPair();
  double elapsed_ticks = t1.tsc - t0.tsc;
  double elapsed_time = (t1.time - t0.time) * 1e-9;
  return elapsed_ticks / elapsed_time;
}

// Measures and returns the TSC frequency by calling
// MeasureTscFrequencyWithSleep(), doubling the sleep interval until the
// frequency measurement stabilizes.
static double MeasureTscFrequency() {
  double last_measurement = -1.0;
  int sleep_nanoseconds = 1000000;  // 1 millisecond.
  for (int i = 0; i < 8; ++i) {
    double measurement = MeasureTscFrequencyWithSleep(sleep_nanoseconds);
    if (measurement * 0.99 < last_measurement &&
        last_measurement < measurement * 1.01) {
      // Use the current measurement if it is within 1% of the
      // previous measurement.
      return measurement;
    }
    last_measurement = measurement;
    sleep_nanoseconds *= 2;
  }
  return last_measurement;
}

#endif  // FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY

    double get_nominal_cpu_frequency() {
        long freq = 0;

        // Google's production kernel has a patch to export the TSC
        // frequency through sysfs. If the kernel is exporting the TSC
        // frequency use that. There are issues where cpuinfo_max_freq
        // cannot be relied on because the BIOS may be exporting an invalid
        // p-state (on x86) or p-states may be used to put the processor in
        // a new mode (turbo mode). Essentially, those frequencies cannot
        // always be relied upon. The same reasons apply to /proc/cpuinfo as
        // well.
        if (read_long_from_file("/sys/devices/system/cpu/cpu0/tsc_freq_khz", &freq)) {
            return freq * 1e3;  // Value is kHz.
        }

#if defined(FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY)
        // On these platforms, the TSC frequency is the nominal CPU
  // frequency.  But without having the kernel export it directly
  // though /sys/devices/system/cpu/cpu0/tsc_freq_khz, there is no
  // other way to reliably get the TSC frequency, so we have to
  // measure it ourselves.  Some CPUs abuse cpuinfo_max_freq by
  // exporting "fake" frequencies for implementing new features. For
  // example, Intel's turbo mode is enabled by exposing a p-state
  // value with a higher frequency than that of the real TSC
  // rate. Because of this, we prefer to measure the TSC rate
  // ourselves on i386 and x86-64.
  return MeasureTscFrequency();
#else

        // If CPU scaling is in effect, we want to use the *maximum*
        // frequency, not whatever CPU speed some random processor happens
        // to be using now.
        if (read_long_from_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", &freq)) {
            return freq * 1e3;  // Value is kHz.
        }

        return 1.0;
#endif  // !FLARE_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY
    }

#endif
}  // namespace flare
