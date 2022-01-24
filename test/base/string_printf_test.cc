
#include <gtest/gtest.h>
#include "flare/base/strings.h"

namespace {

    class BaiduStringPrintfTest : public ::testing::Test {
    protected:
        BaiduStringPrintfTest() {
        };

        virtual ~BaiduStringPrintfTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    TEST_F(BaiduStringPrintfTest, sanity) {
        ASSERT_EQ("hello 1 124 world", flare::base::string_printf("hello %d 124 %s", 1, "world"));
        std::string sth;
        ASSERT_EQ(0, flare::base::string_printf(&sth, "boolean %d", 1));
        ASSERT_EQ("boolean 1", sth);

        ASSERT_EQ(0, flare::base::string_appendf(&sth, "too simple"));
        ASSERT_EQ("boolean 1too simple", sth);
    }

}
