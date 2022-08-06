
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_SYSTEM_NUMA_NUMA_H_
#define FLARE_SYSTEM_NUMA_NUMA_H_

#include <cstdint>
#include <cstddef>
#include "flare/container/dynamic_bitset.h"

namespace flare::system_internal {

    bool is_numa_supported();

    int32_t numa_max_node();

    int32_t numa_max_possible_node();

    int32_t numa_preferred();

    int64_t node_memory_total(int32_t node);

    int64_t node_memory_free(int32_t node);

    int32_t node_page_size();

    void numa_bind(const flare::dynamic_bitset<unsigned long> &nodes);

    void numa_set_interleave_mask(const flare::dynamic_bitset<unsigned long> &nodes);

    void numa_get_interleave_mask(flare::dynamic_bitset<unsigned long> &);

}  // namespace flare::system_internal

#endif  // FLARE_SYSTEM_NUMA_NUMA_H_
