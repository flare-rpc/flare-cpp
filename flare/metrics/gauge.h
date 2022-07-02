//
// Created by liyinbin on 2022/7/1.
//

#ifndef FLARE_METRICS_GAUGE_H_
#define FLARE_METRICS_GAUGE_H_

#include <atomic>
#include "flare/metrics/variable_base.h"
#include "flare/metrics/reducer.h"
#include "flare/metrics/utils/prom_util.h"

namespace flare {

    class gauge : public variable_base {
    public:
        typedef metrics_detail::ReducerSampler<gauge, double, metrics_detail::AddTo<double>,
                metrics_detail::MinusFrom<double> > sampler_type;

        class series_sampler : public metrics_detail::Sampler {
        public:
            typedef metrics_detail::AddTo<double> Op;

            explicit series_sampler(gauge *owner)
                    : _owner(owner), _vector_names(nullptr), _series(Op()) {}

            ~series_sampler() {
                delete _vector_names;
            }

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, _vector_names); }

            void set_vector_names(const std::string &names) {
                if (_vector_names == nullptr) {
                    _vector_names = new std::string;
                }
                *_vector_names = names;
            }

        private:
            gauge *_owner;
            std::string *_vector_names;
            metrics_detail::Series<double, Op> _series;
        };

    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of PassiveStatus.
        explicit gauge(const std::string_view &name,
                       const std::string_view &help,
                       const std::unordered_map<std::string, std::string> &tags = std::unordered_map<std::string, std::string>(), display_filter f = DISPLAY_ON_METRICS)
                : _sampler(nullptr), _series_sampler(nullptr) {
            expose(name, help, tags, f);
        }

        ~gauge() override {
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

        int set_vector_names(const std::string &names) {
            if (_series_sampler) {
                _series_sampler->set_vector_names(names);
                return 0;
            }
            return -1;
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        void collect_metrics(cache_metrics &metric) const override;

        double get_value() const {
            return _value;
        }

        void update(const double value) noexcept {
            _value.store(value);
        }

        void inc() noexcept {
            inc(1.0);
        }

        void inc(const double v) noexcept {
            if (v < 0.0) {
                return;
            }
            change(v);
        }

        void dec() noexcept {
            dec(1.0);
        }

        void dec(const double v) noexcept {
            if (v < 0.0) {
                return;
            }
            change(-1.0 * v);
        }

        sampler_type *get_sampler() {
            if (nullptr == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        metrics_detail::AddTo<double> op() const { return metrics_detail::AddTo<double>(); }

        metrics_detail::MinusFrom<double> inv_op() const { return metrics_detail::MinusFrom<double>(); }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        double reset() {
            FLARE_CHECK(false) << "gauge::reset() should never be called, abort";
            abort();
        }

    protected:

        void change(const double value) noexcept {
            auto current = _value.load();
            while (!_value.compare_exchange_weak(current, current + value));
        }

        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter display_filter) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, display_filter);
            if (rc == 0 &&
                _series_sampler == nullptr &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        std::atomic<double> _value{0.0};
        sampler_type *_sampler;
        series_sampler *_series_sampler;
    };


}  // namespace flare

#endif  // FLARE_METRICS_GAUGE_H_
