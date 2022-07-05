
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_METRICS_HISTOGRAM_H_
#define FLARE_METRICS_HISTOGRAM_H_

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include "flare/metrics/counter.h"
#include "flare/metrics/bucket.h"
#include "flare/metrics/variable_base.h"

namespace flare {


    class histogram : public variable_base {
    public:

        histogram(const std::string &name,
                  const std::string_view &help,
                  const bucket &buckets,
                  const variable_base::tag_type &tags =variable_base::tag_type());

        ~histogram();

        void observe(double value) noexcept;

        void describe(std::ostream &os, bool /*quote_string*/) const override {}

        void collect_metrics(cache_metrics &metric) const override;

    private:
        const bucket _bucket_boundaries;
        std::vector<std::unique_ptr<counter<int64_t>>> _bucket_counts;
        counter<int64_t> _sum;
    };


}  // namespace flare

#endif  // FLARE_METRICS_HISTOGRAM_H_
