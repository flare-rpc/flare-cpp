
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "testing/gtest_wrap.h"
#include <errno.h>                     // errno
#include "flare/files/temp_file.h"

namespace {

class TempFileTest : public ::testing::Test{
protected:
    TempFileTest(){};
    virtual ~TempFileTest(){};
    virtual void SetUp() {
    };
    virtual void TearDown() {
    };
};

TEST_F(TempFileTest, should_create_tmp_file)
{
    flare::temp_file tmp;
    struct stat st;
    //check if existed
    ASSERT_EQ(0, stat(tmp.fname(), &st));
}

TEST_F(TempFileTest, should_write_string)
{
    flare::temp_file tmp;
    const char *exp = "a test file";
    ASSERT_EQ(0, tmp.save(exp));

    FILE *fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp);

    char buf[1024];
    char *act = fgets(buf, 1024, fp);
    fclose(fp);

    EXPECT_STREQ(exp, act);
}

TEST_F(TempFileTest, temp_with_specific_ext)
{
    flare::temp_file tmp("blah");
    const char *exp = "a test file";
    ASSERT_EQ(0, tmp.save(exp));
    struct stat st;
    ASSERT_EQ(0, stat(tmp.fname(), &st));

    ASSERT_STREQ(".blah", strrchr(tmp.fname(), '.'));
    FILE *fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp) << strerror(errno);

    char buf[1024];
    char *act = fgets(buf, 1024, fp);
    fclose(fp);

    EXPECT_STREQ(exp, act);
}

TEST_F(TempFileTest, should_delete_when_exit)
{
    std::string fname;
    struct stat st;
    {
        flare::temp_file tmp;
        //check if existed
        ASSERT_EQ(0, stat(tmp.fname(), &st));
        fname = tmp.fname();
    }

    //check if existed
    ASSERT_EQ(-1, stat(fname.c_str(), &st));
    ASSERT_EQ(ENOENT, errno);
}

TEST_F(TempFileTest, should_save_with_format)
{
    flare::temp_file tmp;
    tmp.save_format("%s%d%ld%s", "justmp", 1, 98L, "hello world");

    FILE *fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp);

    char buf[1024];
    char *act = fgets(buf, 1024, fp);
    fclose(fp);

    EXPECT_STREQ("justmp198hello world", act);
}

TEST_F(TempFileTest, should_save_with_format_in_long_string)
{
    char buf[2048];
    memset(buf, 'a', sizeof(buf));
    buf[2047] = '\0';

    flare::temp_file tmp;
    tmp.save_format("%s", buf);

    FILE *fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp);

    char buf2[2048];
    char *act = fgets(buf2, 2048, fp);
    fclose(fp);    
    EXPECT_STREQ(buf, act);
}

struct test_t {
    int a;
    int b;
    char c[4];
};

TEST_F(TempFileTest, save_binary_twice)
{
    test_t data = {12, -34, {'B', 'E', 'E', 'F'}};
    flare::temp_file tmp;
    ASSERT_EQ(0, tmp.save_bin(&data, sizeof(data)));

    FILE *fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp);
    test_t act_data;
    bzero(&act_data, sizeof(act_data));
    ASSERT_EQ((size_t)1, fread(&act_data, sizeof(act_data), 1, fp));
    fclose(fp);

    ASSERT_EQ(0, memcmp(&data, &act_data, sizeof(data)));

    // save twice
    test_t data2 = { 89, 1000, {'E', 'C', 'A', 'Z'}};
    ASSERT_EQ(0, tmp.save_bin(&data2, sizeof(data2)));
    
    fp = fopen(tmp.fname(), "r");
    ASSERT_NE((void*)0, fp);
    bzero(&act_data, sizeof(act_data));
    ASSERT_EQ((size_t)1, fread(&act_data, sizeof(act_data), 1, fp));
    fclose(fp);
    
    ASSERT_EQ(0, memcmp(&data2, &act_data, sizeof(data2)));

}
}
