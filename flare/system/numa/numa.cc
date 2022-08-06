
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <unistd.h>
#include "flare/system/numa/numa.h"
#include "flare/system/numa/numa_if.h"
#include "flare/log/logging.h"

namespace flare::system_internal {

    static void setup_numa_global();
    static bool g_numa_support = false;
    static bool g_numa_initialized = false;
    static int32_t g_numa_max_node = -1;
    static int32_t g_numa_max_possible_node = -1;
    static std::vector<int32_t> g_numa_node_mem_total;

    void setup_numa_global() {
        g_numa_max_possible_node = get_numa_max_possible_node();
        g_numa_max_node = get_numa_max_node();
        g_numa_initialized = true;
        g_numa_support = get_numa_supported();
    }

    class numa_init_helper {
    public:
        numa_init_helper();
        ~numa_init_helper() = default;
    };

    numa_init_helper::numa_init_helper() {
        setup_numa_global();
    }

    numa_init_helper g_numa_init;

    bool is_numa_supported() {
        return g_numa_support;
    }

    int32_t numa_max_node() {
        return g_numa_max_node;
    }

    int32_t numa_max_possible_node() {
        return g_numa_max_possible_node;
    }

    int32_t numa_preferred() {

    }

    int64_t node_memory_total(int32_t node) {
        FLARE_CHECK(node <= numa_max_node());
        return g_numa_node_mem_total[node];
    }

    int64_t node_memory_free(int32_t node) {

    }

    int numa_pagesize() {
        static int pagesize;
        if (pagesize > 0)
            return pagesize;
        pagesize = getpagesize();
        return pagesize;
    }

}  // namespace flare::system_internal

