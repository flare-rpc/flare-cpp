//
// Created by liyinbin on 2022/4/17.
//


#include "flare/metrics/variable.h"
#include <thread>
#include <atomic>
#include <vector>
#include "testing/gtest_wrap.h"

namespace flare {

    template<class T, class Op>
    void baisc_test(T result) {
        Op t;
        t.update(T(1));
        t.update(T(2));
        t.update(T(3));
        EXPECT_EQ(result, t.read());
        t.reset();
    }

    TEST(BasicOps, Basic) {
        metrics_counter<int> counter;
        counter.add(1);
        counter.add(2);
        counter.add(3);
        EXPECT_EQ(6, counter.read());
        counter.reset();
    }

    TEST(BasicOps, Basic2) {
        metrics_gauge<int> counter;
        counter.add(1);
        counter.add(2);
        counter.subtract(4);
        EXPECT_EQ(-1, counter.read());
        counter.reset();
    }

    TEST(BasicOps, Basic3) {
        baisc_test<int, metrics_miner<int>>
                (1);
        baisc_test<int, metrics_maxer<int>>
                (3);
        baisc_test<int, metrics_averager<int>>
                (2);
    }

    template<class T>
    void update_loop_with_op_test(std::size_t num_threads, std::size_t num_loops,
                              std::size_t num_op, std::function<void(T &)> op,
                              std::function<void(int)> check_func) {
        std::vector<std::thread> threads;
        T adder;
        for (std::size_t i = 1; i <= num_threads; ++i) {
            threads.emplace_back([&, i] {
                for (std::size_t j = 0; j < num_loops * i; ++j) {
                    adder.add(1);
                    if (j * num_op / num_loops == 0) {
                        op(adder);
                    }
                }
            });
        }
        for (auto &t : threads) {
            t.join();
        }
        check_func(adder.read());
    }

    TEST(metrics_variable, read) {
        update_loop_with_op_test<metrics_counter<int>>(
                16, 100000, 10, [](auto &&adder) { adder.read(); },
                [](auto &&final_val) { EXPECT_EQ(13600000, final_val); });
    }

    TEST(metrics_variable, reset) {
        update_loop_with_op_test<metrics_counter<int>>(
                16, 100000, 10, [](auto &&adder) { adder.reset(); },
                [](auto &&final_val) { EXPECT_GT(13600000, final_val); });
    }

    template<class T>
    struct atomic_adder_traits {
        using Type = T;
        using store_buffer = T;
        static constexpr auto kWriteBufferInitializer = T();

        static void update(store_buffer *wb, const T &val) { *wb += val; }

        static void merge(store_buffer *wb1, const store_buffer &wb2) { *wb1 += wb2; }

        static void copy(const store_buffer &src_wb, store_buffer *dst_wb) {
            *dst_wb = src_wb.load();
        }

        static T read(const store_buffer &wb) { return wb.load(); }

        static store_buffer purge(store_buffer *wb) {
            return wb->exchange(kWriteBufferInitializer);
        }
    };

    template<class T>
    class atomic_adder : public metrics_variable<atomic_adder_traits<T>> {
    public:
        void add(const T &value) { metrics_variable<atomic_adder_traits<T>>::update(value); }
    };

    TEST(metrics_variable, Purge) {
        std::atomic<int> total(0);
        update_loop_with_op_test<atomic_adder<std::atomic<int>>>(
                16, 100000, 10, [&](auto &&adder) { total += adder.purge(); },
                [&total](auto &&final_val) { EXPECT_EQ(13600000, total + final_val); });
    }

}  // namespace flare
