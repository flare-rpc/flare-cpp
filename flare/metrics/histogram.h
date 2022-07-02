//
// Created by liyinbin on 2022/7/1.
//

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
#include "flare/metrics/utils/prom_util.h"

namespace flare {


    class histogram : public variable_base {
    public:

        histogram(const std::string &name,
                  const std::string_view &help,
                  const bucket &buckets,
                  const std::unordered_map<std::string, std::string> &tags = std::unordered_map<std::string, std::string>());

        ~histogram();

        void observe(double value) noexcept;

        void describe(std::ostream &os, bool /*quote_string*/) const override {}

        void collect_metrics(cache_metrics &metric) const override;

    private:
        const bucket _bucket_boundaries;
        std::vector<std::unique_ptr<counter>> _bucket_counts;
        counter _sum;
    };


}  // namespace flare

#endif  // FLARE_METRICS_HISTOGRAM_H_
