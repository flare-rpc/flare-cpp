
#ifndef  FLARE_VARIABLE_STATUS_H_
#define  FLARE_VARIABLE_STATUS_H_

#include <string>                       // std::string
#include "flare/base/static_atomic.h"
#include "flare/base/type_traits.h"
#include "flare/strings/str_format.h"
#include "flare/base/lock.h"
#include "flare/metrics/detail/is_atomical.h"
#include "flare/metrics/variable_base.h"

namespace flare {

    // Display a rarely or periodically updated value.
    // Usage:
    //   flare::Status<int> foo_count1(17);
    //   foo_count1.expose("my_value");
    //
    //   flare::Status<int> foo_count2;
    //   foo_count2.set_value(17);
    //
    //   flare::Status<int> foo_count3("my_value", 17);
    template<typename T, typename Enabler = void>
    class Status : public variable_base {
    public:
        Status() {}

        Status(const T &value) : _value(value) {}

        Status(const std::string_view &name, const T &value) : _value(value) {
            this->expose(name);
        }

        Status(const std::string_view &prefix,
               const std::string_view &name, const T &value) : _value(value) {
            this->expose_as(prefix, name);
        }

        // Calling hide() manually is a MUST required by variable_base.
        ~Status() { hide(); }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        T get_value() const {
            flare::base::AutoLock guard(_lock);
            const T res = _value;
            return res;
        }

        void set_value(const T &value) {
            flare::base::AutoLock guard(_lock);
            _value = value;
        }

    private:
        T _value;
        mutable flare::base::Lock _lock;
    };

    template<typename T>
    class Status<T, typename std::enable_if<metrics_detail::is_atomical<T>::value>::type>
            : public variable_base {
    public:
        struct PlaceHolderOp {
            void operator()(T &, const T &) const {}
        };

        class SeriesSampler : public metrics_detail::Sampler {
        public:
            typedef typename std::conditional<
                    true, metrics_detail::AddTo<T>, PlaceHolderOp>
            ::type Op;

            explicit SeriesSampler(Status *owner)
                    : _owner(owner), _series(Op()) {}

            void take_sample() { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, NULL); }

        private:
            Status *_owner;
            metrics_detail::Series<T, Op> _series;
        };

    public:
        Status() : _series_sampler(NULL) {}

        Status(const T &value) : _value(value), _series_sampler(NULL) {}

        Status(const std::string_view &name, const T &value)
                : _value(value), _series_sampler(NULL) {
            this->expose(name, "");
        }

        Status(const std::string_view &prefix,
               const std::string_view &name, const T &value)
                : _value(value), _series_sampler(NULL) {
            this->expose_as(prefix, name, "");
        }

        ~Status() {
            hide();
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = NULL;
            }
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        T get_value() const {
            return _value.load(std::memory_order_relaxed);
        }

        void set_value(const T &value) {
            _value.store(value, std::memory_order_relaxed);
        }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == NULL) {
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
                _series_sampler == NULL &&
                FLAGS_save_series) {
                _series_sampler = new SeriesSampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        std::atomic<T> _value;
        SeriesSampler *_series_sampler;
    };

    // Specialize for std::string, adding a printf-style set_value().
    template<>
    class Status<std::string, void> : public variable_base {
    public:
        Status() {}

        Status(const std::string_view &name, const char *fmt, ...) {
            if (fmt) {
                va_list ap;
                va_start(ap, fmt);
                flare::string_vprintf(&_value, fmt, ap);
                va_end(ap);
            }
            expose(name, "");
        }

        Status(const std::string_view &prefix,
               const std::string_view &name, const char *fmt, ...) {
            if (fmt) {
                va_list ap;
                va_start(ap, fmt);
                flare::string_vprintf(&_value, fmt, ap);
                va_end(ap);
            }
            expose_as(prefix, name, "");
        }

        ~Status() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                os << '"' << get_value() << '"';
            } else {
                os << get_value();
            }
        }

        std::string get_value() const {
            flare::base::AutoLock guard(_lock);
            return _value;
        }

        void set_value(const char *fmt, ...) {
            va_list ap;
            va_start(ap, fmt);
            {
                flare::base::AutoLock guard(_lock);
                flare::string_vprintf(&_value, fmt, ap);
            }
            va_end(ap);
        }

        void set_value(const std::string &s) {
            flare::base::AutoLock guard(_lock);
            _value = s;
        }

    private:
        std::string _value;
        mutable flare::base::Lock _lock;
    };

}  // namespace flare

#endif  // FLARE_VARIABLE_STATUS_H_
