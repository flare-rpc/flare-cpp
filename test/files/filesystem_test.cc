
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <testing/filesystem_test_util.h>
#include "testing/gtest_wrap.h"

TEST(TemporaryDirectory, fsTestTmpdir) {
    flare::file_path tempPath;
    {
        TemporaryDirectory t;
        tempPath = t.path();
        EXPECT_TRUE(flare::exists(flare::file_path(t.path()))
        );
        EXPECT_TRUE(flare::is_directory(t.path())
        );
    }
    EXPECT_TRUE(!flare::exists(tempPath));
}

TEST(filesystem, join_path) {
    flare::file_path prefix = "/user";
    auto lib = flare::join_path(prefix,{"local","lib"});
    ASSERT_EQ(lib.generic_string(), "/user/local/lib");
    auto lib1 = flare::join_path(prefix,{"local","/lib"});
    ASSERT_EQ(lib1.generic_string(), "/lib");
    auto lib2 = flare::join_path({"local","lib"});
    ASSERT_EQ(lib2.generic_string(), "local/lib");
}

