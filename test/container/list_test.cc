

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/container/list.h"
#include "testing/gtest_wrap.h"
#include "testing/test_help.h"
#include <cstddef>
#include <string>

class ContainersListTest : public with_tracked_allocator {
};

TEST_F(ContainersListTest, Empty) {
    flare::list<std::string> list(allocator);
    ASSERT_EQ(list.size(), size_t(0));
}

TEST_F(ContainersListTest, EmplaceOne) {
    flare::list<std::string> list(allocator);
    auto itEntry = list.emplace_front("hello world");
    ASSERT_EQ(*itEntry, "hello world");
    ASSERT_EQ(list.size(), size_t(1));
    auto it = list.begin();
    ASSERT_EQ(it, itEntry);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EmplaceThree) {
    flare::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    ASSERT_EQ(*itA, "a");
    ASSERT_EQ(*itB, "b");
    ASSERT_EQ(*itC, "c");
    ASSERT_EQ(list.size(), size_t(3));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseFront) {
    flare::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itC);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseBack) {
    flare::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itA);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseMid) {
    flare::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itB);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, Grow) {
    flare::list<std::string> list(allocator);
    for (int i = 0; i < 256; i++) {
        list.emplace_front(std::to_string(i));
    }
    ASSERT_EQ(list.size(), size_t(256));
}
