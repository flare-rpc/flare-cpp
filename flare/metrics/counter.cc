//
// Created by liyinbin on 2022/7/3.
//

#include "flare/metrics/counter.h"

namespace flare {

    void counter::collect_metrics(cache_metrics &metric) const {
        copy_metric_family(metric);
        metric.type = metrics_type::mt_counter;
        metric.counter.value = get_value();
    }
}