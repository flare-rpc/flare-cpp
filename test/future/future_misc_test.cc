
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "flare/future/future.h"
#include <array>
#include <queue>


TEST(future_misc, ignored_promise) {
    flare::promise<int> prom;

    (void) prom;
}

TEST(future_misc, prom_filled_future) {
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();
        prom.set_value();
        int dst = 0;
        fut.finally([&](flare::expected<void> v) {
            if (v.has_value()) {
                dst = 1;
            }
        });

        ASSERT_EQ(1, dst);
    }

    {
        flare::promise<int> prom;
        auto fut = prom.get_future();
        prom.set_value(12);

        ASSERT_EQ(12, fut.std_future().get());
    }

    {
        flare::promise<int, std::string> prom;
        auto fut = prom.get_future();
        prom.set_value(12, "hi");

        ASSERT_EQ(std::make_tuple(12, "hi"), fut.std_future().get());
    }
}

TEST(future_misc, simple_then_expect) {
    flare::promise<int> p;
    auto f = p.get_future();

    auto r = f.then_expect([](flare::expected<int> e) { return e.value() * 4; });
    p.set_value(3);

    ASSERT_EQ(r.std_future().get(), 12);
}

TEST(future_misc, prom_post_filled_future) {
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();

        int dst = 0;
        fut.finally([&](flare::expected<void> v) {
            if (v.has_value()) {
                dst = 1;
            }
        });

        prom.set_value();

        ASSERT_EQ(1, dst);
    }

    {
        flare::promise<int> prom;
        auto fut = prom.get_future();

        int dst = 0;
        fut.finally([&](flare::expected<int> v) {
            if (v.has_value()) {
                dst = *v;
            }
        });
        prom.set_value(12);

        ASSERT_EQ(12, dst);
    }

    {
        flare::promise<int, std::string> prom;
        auto fut = prom.get_future();

        int dst = 0;
        std::string str;
        fut.finally([&](flare::expected<int> v, flare::expected<std::string> s) {
            if (v.has_value()) {
                dst = *v;
                str = *s;
            }
        });
        prom.set_value(12, "hi");

        ASSERT_EQ(12, dst);
        ASSERT_EQ("hi", str);
    }
}

TEST(future_misc, simple_then) {
// Post-filled
    {
        flare::promise<int> prom;
        auto fut = prom.get_future();

        auto res = fut.then([&](flare::expected<int> v) { return v.value() + 4; });

        prom.set_value(3);
        ASSERT_EQ(7, res.std_future().get());
    }

// Pre-filled
    {
        flare::promise<int> prom;
        auto fut = prom.get_future();
        prom.set_value(3);

        auto res = fut.then([&](flare::expected<int> v) { return v.value() + 4; });

        ASSERT_EQ(7, res.std_future().get());
    }
}

TEST(future_misc, simple_null_then) {
// Post-filled
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();

        auto res = fut.then([&]() { return 4; });

        prom.set_value();
        ASSERT_EQ(4, res.std_future().get());
    }

// Pre-filled
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();
        prom.set_value();

        auto res = fut.then([&]() { return 4; });

        ASSERT_EQ(4, res.std_future().get());
    }
}

TEST(future_misc, simple_null_then_exptec) {
// Post-filled
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();

        auto res = fut.then_expect([&](flare::expected<void>) { return 4; });

        prom.set_value();
        ASSERT_EQ(4, res.std_future().get());
    }

// Pre-filled
    {
        flare::promise<void> prom;
        auto fut = prom.get_future();
        prom.set_value();

        auto res = fut.then_expect([&](flare::expected<void>) { return 4; });

        ASSERT_EQ(4, res.std_future().get());
    }
}

TEST(future_misc, simple_then_failure) {
// Post-filled
    {
        flare::promise<int> prom;
        auto fut = prom.get_future();

        auto res = fut.then([&](flare::expected<int> v) { return v.value() + 4; });

        prom.set_exception(std::make_exception_ptr(std::runtime_error("nope")));
        ASSERT_THROW(res.std_future().get(), std::runtime_error);
    }

// Pre-filled
    {
        flare::promise<int> prom;
        auto fut = prom.get_future();
        prom.set_exception(std::make_exception_ptr(std::runtime_error("nope")));

        auto res = fut.then([&](flare::expected<int> v) { return v.value() + 4; });

        ASSERT_THROW(res.std_future().get(), std::runtime_error);
    }
}

TEST(future_misc, Forgotten_promise) {
    flare::future<int> fut;
    {
        flare::promise<int> prom;
        fut = prom.get_future();
    }

    ASSERT_THROW(fut.std_future().get(), flare::unfull_filled_promise);
}

TEST(future_misc, simple_get) {
    flare::promise<int> prom;
    auto fut = prom.get_future();

    prom.set_value(3);
    ASSERT_EQ(3, fut.std_future().get());
}

TEST(future_misc, simple_join) {
    flare::promise<int> p_a;
    flare::promise<std::string> p_b;

    auto f =
            join(p_a.get_future(), p_b.get_future()).then([](int a, std::string) {
                return a;
            });
    p_a.set_value(3);
    p_b.set_value("yo");

    ASSERT_EQ(3, f.std_future().get());
}

TEST(future_misc, partial_join_failure) {
    flare::promise<int> p_a;
    flare::promise<std::string> p_b;

    int dst = 0;
    join(p_a.get_future(), p_b.get_future())
            .finally([&](flare::expected<int> a, flare::expected<std::string> b) {
                dst = a.value();
                ASSERT_FALSE(b.has_value());
            });
    ASSERT_EQ(0, dst);
    p_a.set_value(3);
    ASSERT_EQ(0, dst);
    p_b.set_exception(std::make_exception_ptr(std::runtime_error("nope")));

    ASSERT_EQ(3, dst);
}

TEST(future_misc, handler_returning_future) {
    flare::promise<int> p;
    auto f = p.get_future();

    auto f2 = f.then([](int x) {
        flare::promise<int> p;
        auto f = p.get_future();

        p.set_value(x);
        return f;
    });

    p.set_value(3);

    ASSERT_EQ(3, f2.std_future().get());
}

TEST(future_misc, void_promise) {
    flare::promise<void> prom;
    auto fut = prom.get_future();

    int dst = 0;
    fut.finally([&](flare::expected<void> v) {
        ASSERT_TRUE(v.has_value());
        dst = 4;
    });

    prom.set_value();
    ASSERT_EQ(4, dst);
}

TEST(future_misc, variadic_get) {
    static_assert(std::is_same_v<flare::future<void>::value_type, void>);
    static_assert(std::is_same_v<flare::future<int>::value_type, int>);
    static_assert(std::is_same_v<flare::future<void, void>::value_type, void>);
    static_assert(
            std::is_same_v<flare::future<int, int>::value_type, std::tuple<int, int>>);
    static_assert(std::is_same_v<flare::future<int, void>::value_type, int>);
    static_assert(std::is_same_v<flare::future<void, int>::value_type, int>);
    static_assert(std::is_same_v<flare::future<void, int, void>::value_type, int>);
    static_assert(
            std::is_same_v<flare::future<int, void, int>::value_type, std::tuple<int, int>>);
}

TEST(future_misc, variadic_get_failure) {
    flare::promise<void, void> p;
    auto f = p.get_future();

    p.set_exception(std::make_exception_ptr(std::runtime_error("dead")));

    ASSERT_THROW(f.get(), std::runtime_error);
}

TEST(future_misc, segmented_callback) {

    flare::promise<void> p;
    auto f = p.get_future().then([]() { return flare::segmented(12, 12); }).then([](int a, int b) { return a + b; });

    p.set_value();

    ASSERT_EQ(24, f.get());
}

TEST(future_misc, defered_returned_future) {
    flare::promise<int> p;

    auto f = p.get_future().then([](int) {
        flare::promise<int> final_p;

        auto result = final_p.get_future();
        std::thread w(
                [final_p = std::move(final_p)]() mutable { final_p.set_value(15); });
        w.detach();
        return result;
    });

    p.set_value(1);
    ASSERT_EQ(f.get(), 15);
}