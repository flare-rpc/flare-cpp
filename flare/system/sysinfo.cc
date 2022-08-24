
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/system/sysinfo.h"
#include "flare/times/time.h"

namespace flare {
    void sysinfo::mem_calc_ram(mem_info& mem) {
        int64_t total = mem.total / 1024, diff;
        uint64_t lram = (mem.total / (1024 * 1024));
        int ram = (int)lram; /* must cast after division */
        int remainder = ram % 8;

        if (remainder > 0) {
            ram += (8 - remainder);
        }

        mem.ram = ram;

        diff = total - (mem.actual_free / 1024);
        mem.used_percent = (double)(diff * 100) / total;

        diff = total - (mem.actual_used / 1024);
        mem.free_percent = (double)(diff * 100) / total;
    }

    result_status sysinfo::get_proc_cpu(flare_pid_t pid, proc_cpu_info &proccpu) {
        const auto time_now = flare::time_now().to_unix_millis();
        proc_cpu_info prev = {};
        auto iter = process_cache.find(pid);
        const bool found = iter != process_cache.end();
        if (found) {
            prev = iter->second;
        }

        auto status = get_proc_time(pid, *(proc_time_info*)&proccpu);
        if (status != 0) {
            if (found) {
                process_cache.erase(iter);
            }
            return result_status::success();
        }

        proccpu.last_time = time_now;
        if (!found || (prev.start_time != proccpu.start_time)) {
            // This is a new process or a different process we have in the cache
            process_cache[pid] = proccpu;
            return result_status::success();
        }

        auto time_diff = time_now - prev.last_time;
        if (!time_diff) {
            // we don't want divide by zero
            time_diff = 1;
        }
        proccpu.percent = (proccpu.total - prev.total) / (double)time_diff;
        process_cache[pid] = proccpu;

        return result_status::success();
    }
}  // namespace flare