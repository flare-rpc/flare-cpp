
#include <gtest/gtest.h>
#include <flare/fiber/fiber_latch.h>
#include "flare/base/static_atomic.h"
#include "flare/base/time.h"

namespace {
    struct Arg {
        flare::fiber::fiber_latch latcher;
        std::atomic<int> num_sig;
    };

    void *signaler(void *arg) {
        Arg *a = (Arg *) arg;
        a->num_sig.fetch_sub(1, std::memory_order_relaxed);
        a->latcher.signal();
        return NULL;
    }

    TEST(CountdonwEventTest, sanity) {
        for (int n = 1; n < 10; ++n) {
            Arg a;
            a.num_sig = n;
            a.latcher.reset(n);
            for (int i = 0; i < n; ++i) {
                fiber_id_t tid;
                ASSERT_EQ(0, fiber_start_urgent(&tid, NULL, signaler, &a));
            }
            a.latcher.wait();
            ASSERT_EQ(0, a.num_sig.load(std::memory_order_relaxed));
        }
    }

    TEST(CountdonwEventTest, timed_wait) {
        flare::fiber::fiber_latch latcher;
        int rc = latcher.timed_wait(flare::base::milliseconds_from_now(100));
        ASSERT_EQ(rc, ETIMEDOUT);
        latcher.signal();
        rc = latcher.timed_wait(flare::base::milliseconds_from_now(100));
        ASSERT_EQ(rc, 0);
        flare::fiber::fiber_latch latcher1;
        latcher1.signal();
        rc = latcher.timed_wait(flare::base::milliseconds_from_now(1));
        ASSERT_EQ(rc, 0);
    }
} // namespace
