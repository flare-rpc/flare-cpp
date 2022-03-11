//
// Created by liyinbin on 2022/3/6.
//

#include <gtest/gtest.h>
#include "flare/files/readline_file.h"
#include "flare/files/temp_file.h"
#include "flare/strings/fmt/format.h"

namespace flare {

    TEST(readlinefile, all) {
        temp_file lines_file("txt");
        std::string content;
        for(int i = 0; i < 100; i++) {
            content += fmt::format("line{}:flare{}\n", i, i);
        }

        content +=" \n\n\t\n\n";

        content +="\\n\n";
        content +="\\n\r\n";
        content +="\\r\\n\r\n";
        for(int i = 0; i < 100; i++) {
            content += fmt::format("line{}:flare{}\r\n", i, i);
        }

        lines_file.save_bin(content.data(), content.size());
        {
            readline_file rl_file;
            auto rs = rl_file.open(lines_file.fname());
            EXPECT_TRUE(rs.ok());
            EXPECT_EQ(205UL, rl_file.size());
        }
        {
            readline_file rl_file1;
            auto rs = rl_file1.open(lines_file.fname(), flare::readline_option::eNoSkip);
            EXPECT_TRUE(rs.ok());
            EXPECT_EQ(207UL, rl_file1.size());
        }

        {
            readline_file rl_file1;
            auto rs = rl_file1.open(lines_file.fname(), flare::readline_option::eTrimWhitespace);
            EXPECT_TRUE(rs.ok());
            EXPECT_EQ(203UL, rl_file1.size());
        }
    }
}