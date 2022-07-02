//
// Created by liyinbin on 2022/7/3.
//

#include "flare/metrics/gauge.h"

namespace flare {

    void gauge::collect_metrics(cache_metrics &metric) const {
        copy_metric_family(metric);
        metric.type = metrics_type::mt_gauge;
        metric.gauge.value = _value;
    }
}