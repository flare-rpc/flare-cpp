//
// Created by liyinbin on 2022/3/15.
//


#include "flare/log/logging.h"

#include <string>
#include <vector>

#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

namespace foreign_ns {

    struct AwesomeLogSink : public flare::log::log_sink {
        void send(flare::log::log_severity severity, const char *full_filename,
                  const char *base_filename, int line, const struct ::tm *tm_time,
                  const char *message, size_t message_len) override {
            msgs.emplace_back(message, message_len);
        }

        std::vector<std::string> msgs;
    };

    std::string my_prefix, my_prefix2;

    void WriteLoggingPrefix(std::string *s) { *s += my_prefix; }

    void WriteLoggingPrefix2(std::string *s) { *s += my_prefix2; }

    TEST(Logging, Prefix) {
        AwesomeLogSink sink;
        flare::log::add_log_sink(&sink);

        FLARE_LOG_INFO("something");

        my_prefix = "[prefix]";
        FLARE_LOG_INFO("something");

        my_prefix = "[prefix1]";
        FLARE_LOG_INFO("something");

        my_prefix2 = "[prefix2]";
        FLARE_LOG_INFO("something");

        ASSERT_THAT(sink.msgs,
                    ::testing::ElementsAre("something", "[prefix] something",
                                           "[prefix1] something",
                                           "[prefix1] [prefix2] something"));
        flare::log::remove_log_sink(&sink);
    }

}  // namespace foreign_ns

FLARE_INTERNAL_LOGGING_REGISTER_PREFIX_PROVIDER(0,
                                                foreign_ns::WriteLoggingPrefix)

FLARE_INTERNAL_LOGGING_REGISTER_PREFIX_PROVIDER(1,
                                                foreign_ns::WriteLoggingPrefix2)
