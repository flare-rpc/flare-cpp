//
// Created by liyinbin on 2022/7/1.
//

#ifndef FLARE_METRICS_GAUGE_H_
#define FLARE_METRICS_GAUGE_H_

#include <atomic>
#include <type_traits>
#include "flare/metrics/variable_base.h"
#include "flare/metrics/variable_reducer.h"

namespace flare {

    template<typename T>
    class gauge : public variable_reducer<T, metrics_detail::AddTo<T>, metrics_detail::MinusFrom<T> > {
    public:
        typedef variable_reducer <T, metrics_detail::AddTo<T>, metrics_detail::MinusFrom<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        gauge() : Base() {}

        explicit gauge(const std::string_view &name,
                       const std::string_view &help = "",
                       const variable_base::tag_type &tags = variable_base::tag_type(),
                       display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose(name, help, tags, f);
        }

        gauge(const std::string_view &prefix,
              const std::string_view &name,
              const std::string_view &help = "",
              const variable_base::tag_type &tags = variable_base::tag_type(),
              display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose_as(prefix, name, help, tags, f);
        }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

        ~gauge() { variable_base::hide(); }
    };

    template<typename T>
    class max_gauge : public variable_reducer<T, metrics_detail::MaxTo<T> > {
    public:
        typedef variable_reducer <T, metrics_detail::MaxTo<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        max_gauge() : Base(std::numeric_limits<T>::min()) {}

        explicit max_gauge(const std::string_view &name,
                           const std::string_view &help = "",
                           const variable_base::tag_type &tags = variable_base::tag_type(),
                           display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name, help, tags, f);
        }

        max_gauge(const std::string_view &prefix, const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name, help, tags, f);
        }

        ~max_gauge() { variable_base::hide(); }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

    private:

        friend class metrics_detail::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit max_gauge(T default_value) : Base(default_value) {
        }

        max_gauge(T default_value, const std::string_view &prefix,
                  const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)
        )
                : Base(default_value) {
            this->expose_as(prefix, name, help, tags, f);
        }

        max_gauge(T default_value, const std::string_view &name) : Base(default_value) {
            this->expose(name);
        }
    };

    template<typename T>
    class min_gauge : public variable_reducer<T, metrics_detail::MinTo<T> > {
    public:
        typedef variable_reducer <T, metrics_detail::MinTo<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        min_gauge() : Base(std::numeric_limits<T>::max()) {}

        explicit min_gauge(const std::string_view &name,
                           const std::string_view &help = "",
                           const variable_base::tag_type &tags = variable_base::tag_type(),
                           display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name, help, tags, f);
        }

        min_gauge(const std::string_view &prefix, const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name, help, tags, f);
        }

        ~min_gauge() { variable_base::hide(); }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

    private:

        friend class metrics_detail::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit min_gauge(T default_value) : Base(default_value) {
        }

        min_gauge(T default_value, const std::string_view &prefix,
                  const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)
        )
                : Base(default_value) {
            this->expose_as(prefix, name, help, tags, f);
        }

        min_gauge(T default_value, const std::string_view &name) : Base(default_value) {
            this->expose(name);
        }
    };

    template<typename Tp>
    class status_gauge : public variable_base {
    public:
        typedef Tp value_type;
        typedef metrics_detail::reducer_sampler<status_gauge, Tp, metrics_detail::AddTo<Tp>,
                metrics_detail::MinusFrom<Tp> > sampler_type;

        struct PlaceHolderOp {
            void operator()(Tp &, const Tp &) const {}
        };

        static const bool ADDITIVE = (std::is_integral<Tp>::value ||
                                      std::is_floating_point<Tp>::value ||
                                      is_vector<Tp>::value);

        class series_sampler : public metrics_detail::variable_sampler {
        public:
            typedef typename std::conditional<
                    ADDITIVE, metrics_detail::AddTo<Tp>, PlaceHolderOp>::type Op;

            explicit series_sampler(status_gauge *owner)
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
            status_gauge *_owner;
            std::string *_vector_names;
            metrics_detail::Series<Tp, Op> _series;
        };

    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of status_gauge.
        status_gauge(const std::string_view &name,
                     Tp (*getfn)(void *), void *arg,
                     const std::string_view &help = "",
                     const variable_base::tag_type &tags = variable_base::tag_type(),
                     display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
            expose(name, help, tags, make_filter(f));
        }

        status_gauge(const std::string_view &prefix,
                     const std::string_view &name,
                     Tp (*getfn)(void *), void *arg,
                     const std::string_view &help = "",
                     const variable_base::tag_type &tags = variable_base::tag_type(),
                     display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
            expose_as(prefix, name, help, tags, make_filter(f));
        }

        status_gauge(Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
        }

        ~status_gauge() {
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

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        Tp get_value() const {
            return (_getfn ? _getfn(_arg) : Tp());
        }

        sampler_type *get_sampler() {
            if (nullptr == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        metrics_detail::AddTo<Tp> op() const { return metrics_detail::AddTo<Tp>(); }

        metrics_detail::MinusFrom<Tp> inv_op() const { return metrics_detail::MinusFrom<Tp>(); }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        Tp reset() {
            FLARE_CHECK(false) << "status_gauge::reset() should never be called, abort";
            abort();
        }

    protected:

        display_filter make_filter(display_filter f) {
            if(!std::is_integral<Tp>::value && !std::is_floating_point<Tp>::value) {
                return static_cast<display_filter>(f & ~DISPLAY_ON_METRICS);
            }
            return f;
        }
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter display_filter) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, display_filter);
            if (ADDITIVE &&
                rc == 0 &&
                _series_sampler == nullptr &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:

        Tp (*_getfn)(void *);

        void *_arg;
        sampler_type *_sampler;
        series_sampler *_series_sampler;
    };

    template<typename Tp> const bool status_gauge<Tp>::ADDITIVE;


}  // namespace flare

#endif  // FLARE_METRICS_GAUGE_H_
