//
// Created by liyinbin on 2022/7/1.
//

#ifndef FLARE_METRICS_COUNTER_H_
#define FLARE_METRICS_COUNTER_H_

#include <limits>                                 // std::numeric_limits
#include "flare/log/logging.h"                         // FLARE_LOG()
#include "flare/base/type_traits.h"                     // flare::base::add_cr_non_integral
#include "flare/base/class_name.h"                      // class_name_str
#include "flare/metrics/variable_base.h"                        // variable_base
#include "flare/metrics/detail/combiner.h"                 // metrics_detail::agent_combiner
#include "flare/metrics/detail/sampler.h"                  // ReducerSampler
#include "flare/metrics/detail/series.h"
#include "flare/metrics/window.h"
#include "flare/metrics/utils/prom_util.h"
#include "flare/metrics/reducer.h"

namespace flare {

    class counter : public variable_base {
    public:
        typedef typename metrics_detail::agent_combiner<uint64_t, uint64_t, metrics_detail::AddTo<uint64_t>> combiner_type;
        typedef typename combiner_type::Agent agent_type;
        typedef metrics_detail::ReducerSampler<counter, uint64_t, metrics_detail::AddTo<uint64_t>, metrics_detail::MinusFrom<uint64_t>> sampler_type;

        class series_sampler : public metrics_detail::Sampler {
        public:
            series_sampler(counter *owner, const metrics_detail::AddTo<uint64_t> &op)
                    : _owner(owner), _series(op) {}

            ~series_sampler() {}

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, nullptr); }

        private:
            counter *_owner;
            metrics_detail::Series<uint64_t, metrics_detail::AddTo<uint64_t>> _series;
        };

    public:
        // The `identify' must satisfy: identity Op a == a
        counter(const std::string_view &name,
                const std::string_view &help,
                const std::unordered_map<std::string, std::string> &tags = std::unordered_map < std::string, std::string

        >(),
        display_filter f = DISPLAY_ON_METRICS,
                uint64_t
        identity = 0ul,
        const metrics_detail::AddTo<uint64_t> &op = metrics_detail::AddTo<uint64_t>(),
        const metrics_detail::MinusFrom<uint64_t> &inv_op = metrics_detail::MinusFrom<uint64_t>()
        )
        :
        _combiner(identity, identity, op
        ), _sampler(nullptr), _series_sampler(nullptr),
        _inv_op(inv_op) {
                expose(name, help, tags, f);
        }

        ~counter() override {
            // Calling hide() manually is a MUST required by variable_base.
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = nullptr;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = nullptr;
            }
        }

        // Add a value.
        // Returns self reference for chaining.
        counter &operator<<(uint64_t value);

        // Get reduced value.
        // Notice that this function walks through threads that ever add values
        // into this reducer. You should avoid calling it frequently.
        uint64_t get_value() const {
            return _combiner.combine_agents();
        }


        // Reset the reduced value to uint64_t().
        // Returns the reduced value before reset.
        uint64_t reset() { return _combiner.reset_all_agents(); }

        void describe(std::ostream &os, bool quote_string) const override {
            os << get_value();
        }

        void collect_metrics(cache_metrics &metric) const override;

        // True if this reducer is constructed successfully.
        bool valid() const { return _combiner.valid(); }

        // Get instance of Op.
        const metrics_detail::AddTo<uint64_t> &op() const { return _combiner.op(); }

        const metrics_detail::MinusFrom<uint64_t> &inv_op() const { return _inv_op; }

        sampler_type *get_sampler() {
            if (nullptr == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

    protected:

        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter display_filter) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, display_filter);
            if (rc == 0 &&
                _series_sampler == nullptr &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this, _combiner.op());
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        combiner_type _combiner;
        sampler_type *_sampler;
        series_sampler *_series_sampler;
        metrics_detail::MinusFrom<uint64_t> _inv_op;
    };

    inline counter &counter::operator<<(uint64_t value) {
        // It's wait-free for most time
        agent_type *agent = _combiner.get_or_create_tls_agent();
        if (__builtin_expect(!agent, 0)) {
            FLARE_LOG(FATAL) << "Fail to create agent";
            return *this;
        }
        agent->element.modify(_combiner.op(), value);
        return *this;
    }


}  // namespace flare

#endif  // FLARE_METRICS_COUNTER_H_
