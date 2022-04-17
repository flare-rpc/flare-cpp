//
// Created by liyinbin on 2022/4/17.
//

#ifndef FLARE_METRICS_VARIABLE_H_
#define FLARE_METRICS_VARIABLE_H_

#include <memory>
#include <mutex>

#include "flare/base/profile.h"
#include "flare/thread/thread_local.h"

namespace flare {

    template<class Traits>
    class metrics_variable {
        using T = typename Traits::Type;

    public:
        metrics_variable()
                : _tls_buffer(
                [this]() { return std::make_unique<store_buffer_wrapper>(this); }) {
            Traits::copy(Traits::kWriteBufferInitializer, &_exited_thread_combined);
        }

        void update(const T &value) noexcept {
            Traits::update(&_tls_buffer.get()->buffer_, value);
        }

        // Full read and reset atomically.
        // Only if Traits implements purge method, you can call this method.
        T purge() noexcept { return purge_helper<Traits>(nullptr); }

        // Not atomically
        void reset() noexcept {
            {
                std::unique_lock<std::mutex> lock(mutex);
                _exited_thread_combined = Traits::kWriteBufferInitializer;
            }
            _tls_buffer.for_each([&](auto &&wrapper) {
                wrapper->buffer_ = Traits::kWriteBufferInitializer;
            });
        }

        T read() const noexcept {
            store_buffer wb;
            Traits::copy(_exited_thread_combined, &wb);
            _tls_buffer.for_each(
                    [&](auto &&wrapper) { Traits::merge(&wb, wrapper->buffer_); });
            return Traits::read(wb);
        }

    private:
        using store_buffer = typename Traits::store_buffer;

        template<typename C>
        T purge_helper(decltype(&C::purge)) {
            store_buffer wb = Traits::purge(&_exited_thread_combined);
            _tls_buffer.for_each([&](auto &&wrapper) {
                Traits::merge(&wb, Traits::purge(&wrapper->buffer_));
            });
            return Traits::read(wb);
        }

        // merge to _exited_thread_combined when thread exits.
        struct alignas(hardware_destructive_interference_size) store_buffer_wrapper {
            explicit store_buffer_wrapper(metrics_variable *parent) : parent_(parent) {
                Traits::copy(Traits::kWriteBufferInitializer, &buffer_);
            }

            ~store_buffer_wrapper() {
                std::unique_lock<std::mutex> lock(parent_->mutex);
                Traits::merge(&parent_->_exited_thread_combined, buffer_);
            }

            metrics_variable *parent_;
            store_buffer buffer_;
        };

        mutable store_buffer _exited_thread_combined;
        mutable std::mutex mutex;
        thread_local_store<store_buffer_wrapper> _tls_buffer;
    };


    namespace metrics_internal {

        // Wrapper of single value T with different Op.
        // Inspired from bvar : For performance issues, we don't let Op return value,
        // instead it shall set the result to the first parameter in-place.
        // Namely to add two values "+=" should be implemented rather than "+".
        template<class T, class Op>
        struct cumulative_traits {
            using Type = T;
            using store_buffer = std::atomic<T>;
            static constexpr auto kWriteBufferInitializer = Op::kIdentity;

            static void update(store_buffer *wb, const T &val) { Op()(wb, val); }

            static void merge(store_buffer *wb1, const store_buffer &wb2) {
                Op()(wb1, wb2);
            }

            static void copy(const store_buffer &src_wb, store_buffer *dst_wb) {
                dst_wb->store(src_wb.load(std::memory_order_relaxed),
                              std::memory_order_relaxed);
            }

            static T read(const store_buffer &wb) {
                return wb.load(std::memory_order_relaxed);
            }
        };

        template<class T>
        struct op_add {
            static constexpr auto kIdentity = T();

            void operator()(std::atomic<T> *l, const std::atomic<T> &r) const {
                // No costly RMW here.
                l->store(
                        l->load(std::memory_order_relaxed) + r.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
            }
        };

        template<class T>
        using add_traits = cumulative_traits<T, op_add<T>>;

        template<class T>
        struct op_min {
            static constexpr auto kIdentity = std::numeric_limits<T>::max();

            void operator()(std::atomic<T> *l, const std::atomic<T> &r) {
                if (auto v = r.load(std::memory_order_relaxed);
                        v < l->load(std::memory_order_relaxed)) {
                    l->store(v, std::memory_order_relaxed);
                }
            }
        };

        template<class T>
        using min_traits = cumulative_traits<T, op_min<T>>;

        template<class T>
        struct op_max {
            static constexpr auto kIdentity = std::numeric_limits<T>::min();

            void operator()(std::atomic<T> *l, const std::atomic<T> &r) const {
                if (auto v = r.load(std::memory_order_relaxed);
                        v > l->load(std::memory_order_relaxed)) {
                    l->store(v, std::memory_order_relaxed);
                }
            }
        };

        template<class T>
        using max_traits = cumulative_traits<T, op_max<T>>;

        template<class T>
        struct avg_traits {
            struct avg_buffer {
                T val_;
                std::size_t num_;
            };
            using Type = T;
            using store_buffer = avg_buffer;
            static constexpr auto kWriteBufferInitializer = avg_buffer{T(), 0};

            static void update(store_buffer *wb, const T &val) {
                wb->val_ += val;
                ++wb->num_;
            }

            static void merge(store_buffer *wb1, const store_buffer &wb2) {
                wb1->val_ += wb2.val_;
                wb1->num_ += wb2.num_;
            }

            static void copy(const store_buffer &src_wb, store_buffer *dst_wb) {
                *dst_wb = src_wb;
            }

            static T read(const store_buffer &wb) {
                return wb.num_ ? (wb.val_ / wb.num_) : 0;
            }
        };

    }  // namespace metrics_internal

    // An optimized-for-writer thread-safe counter.
    //
    // I don't see a point in using distinct class for "Counter" and "Gauge", but to
    // keep the naming across our library consistent, let's separate them.
    template<class T, class Base = metrics_variable<metrics_internal::add_traits<T>>>
    class metrics_counter : private Base {
    public:
        void add(T value) noexcept {
            FLARE_CHECK(value >= 0);
            Base::update(value);
        }

        void increment() noexcept { add(1); }

        T read() const noexcept { return Base::read(); }

        void reset() noexcept { Base::reset(); }  // NOT thread-safe.
    };

    // Same as `metrics_counter` except that values in it can be decremented.
    template<class T, class Base = metrics_variable<metrics_internal::add_traits<T>>>
    class metrics_gauge : private Base {
    public:
        void add(T value) noexcept {
            FLARE_CHECK(value >= 0);
            Base::update(value);
        }

        void subtract(T value) noexcept {
            FLARE_CHECK(value >= 0);
            Base::update(-value);
        }

        void increment() noexcept { add(1); }

        void decrement() noexcept { subtract(1); }

        T read() const noexcept { return Base::read(); }

        void reset() noexcept { Base::reset(); }  // NOT thread-safe.
    };

    // An optimized-for-writer thread-safe minimizer.
    template<class T, class Base = metrics_variable<metrics_internal::min_traits<T>>>
    class metrics_miner : private Base {
    public:
        void update(T value) noexcept { Base::update(value); }

        T read() const noexcept { return Base::read(); }

        void reset() noexcept { Base::reset(); }  // NOT thread-safe.
    };

    // An optimized-for-writer thread-safe maximizer.
    template<class T, class Base = metrics_variable<metrics_internal::max_traits<T>>>
    class metrics_maxer : private Base {
    public:
        void update(T value) noexcept { Base::update(value); }

        T read() const noexcept { return Base::read(); }

        void reset() noexcept { Base::reset(); }  // NOT thread-safe.
    };

    // An optimized-for-writer thread-safe averager.
    template<class T, class Base = metrics_variable<metrics_internal::avg_traits<T>>>
    class metrics_averager : private Base {
    public:
        void update(T value) noexcept { Base::update(value); }

        T read() const noexcept { return Base::read(); }

        void reset() noexcept { Base::reset(); }  // NOT thread-safe.
    };


}  // namespace flare

#endif // FLARE_METRICS_VARIABLE_H_
