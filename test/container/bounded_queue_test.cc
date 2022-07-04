

#include <iostream>
#include "flare/container/bounded_queue.h"
#include "testing/gtest_wrap.h"

namespace {

    TEST(BoundedQueueTest, sanity) {
        const int N = 36;
        char storage[N * sizeof(int)];
        flare::container::bounded_queue<int> q(storage, sizeof(storage), flare::container::NOT_OWN_STORAGE);
        ASSERT_EQ(0ul, q.size());
        ASSERT_TRUE(q.empty());
        ASSERT_TRUE(NULL == q.top());
        ASSERT_TRUE(NULL == q.bottom());
        for (int i = 1; i <= N; ++i) {
            if (i % 2 == 0) {
                ASSERT_TRUE(q.push(i));
            } else {
                int *p = q.push();
                ASSERT_TRUE(p);
                *p = i;
            }
            ASSERT_EQ((size_t) i, q.size());
            ASSERT_EQ(1, *q.top());
            ASSERT_EQ(i, *q.bottom());
        }
        ASSERT_FALSE(q.push(N + 1));
        ASSERT_FALSE(q.push_top(N + 1));
        ASSERT_EQ((size_t) N, q.size());
        ASSERT_FALSE(q.empty());
        ASSERT_TRUE(q.full());

        for (int i = 1; i <= N; ++i) {
            ASSERT_EQ(i, *q.top());
            ASSERT_EQ(N, *q.bottom());
            if (i % 2 == 0) {
                int tmp = 0;
                ASSERT_TRUE(q.pop(&tmp));
                ASSERT_EQ(tmp, i);
            } else {
                ASSERT_TRUE(q.pop());
            }
            ASSERT_EQ((size_t) (N - i), q.size());
        }
        ASSERT_EQ(0ul, q.size());
        ASSERT_TRUE(q.empty());
        ASSERT_FALSE(q.full());
        ASSERT_FALSE(q.pop());

        for (int i = 1; i <= N; ++i) {
            if (i % 2 == 0) {
                ASSERT_TRUE(q.push_top(i));
            } else {
                int *p = q.push_top();
                ASSERT_TRUE(p);
                *p = i;
            }
            ASSERT_EQ((size_t) i, q.size());
            ASSERT_EQ(i, *q.top());
            ASSERT_EQ(1, *q.bottom());
        }
        ASSERT_FALSE(q.push(N + 1));
        ASSERT_FALSE(q.push_top(N + 1));
        ASSERT_EQ((size_t) N, q.size());
        ASSERT_FALSE(q.empty());
        ASSERT_TRUE(q.full());

        for (int i = 1; i <= N; ++i) {
            ASSERT_EQ(N, *q.top());
            ASSERT_EQ(i, *q.bottom());
            if (i % 2 == 0) {
                int tmp = 0;
                ASSERT_TRUE(q.pop_bottom(&tmp));
                ASSERT_EQ(tmp, i);
            } else {
                ASSERT_TRUE(q.pop_bottom());
            }
            ASSERT_EQ((size_t) (N - i), q.size());
        }
        ASSERT_EQ(0ul, q.size());
        ASSERT_TRUE(q.empty());
        ASSERT_FALSE(q.full());
        ASSERT_FALSE(q.pop());
    }

} // anonymous namespace
