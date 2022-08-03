//
// Created by liyinbin on 2022/8/3.
//

#ifndef FLARE_SYSTEM_NUMA_NUMA_H_
#define FLARE_SYSTEM_NUMA_NUMA_H_

#include <cstdint>
#include <cstddef>
#include "flare/container/bitset.h"

namespace flare::system_internal {

    bool is_numa_supported();

    int32_t numa_max_node();

    int32_t numa_max_possible_node();

    int32_t numa_preferred();

    int64_t node_memory_total(int32_t node);

    int64_t node_memory_free(int32_t node);

    int32_t node_page_size();

    void numa_bind(const flare::bitmap &nodes);

    void numa_set_interleave_mask(const flare::bitmap &nodes);

    void numa_get_interleave_mask(flare::bitmap &);

}  // namespace flare::system_internal

#endif  // FLARE_SYSTEM_NUMA_NUMA_H_
