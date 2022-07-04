//
// Created by liyinbin on 2022/1/27.
//

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
    EXPECT_TRUE(!
                        flare::exists(tempPath)
    );
}
