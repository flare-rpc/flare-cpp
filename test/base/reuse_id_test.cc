//
// Created by liyinbin on 2022/4/4.
//

#include "flare/base/reuse_id.h"

#include "gtest/gtest.h"

struct fd_tag;
struct fd_tag1;

TEST(reuse_id, all) {

    auto id = flare::reuse_id::instance<fd_tag>();
    auto id1 = flare::reuse_id::instance<fd_tag1>();
    EXPECT_EQ(0UL, id->next());
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(2UL, id->next());
    EXPECT_EQ(0UL, id1->next());
    id->free(1);
    EXPECT_EQ(1UL, id->next());
    EXPECT_EQ(1UL, id1->next());

}