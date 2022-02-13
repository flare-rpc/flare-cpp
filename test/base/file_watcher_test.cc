
#include <gtest/gtest.h>
#include "flare/base/file_watcher.h"
#include "flare/log/logging.h"

namespace {
    class FileWatcherTest : public ::testing::Test {
    protected:
        FileWatcherTest() {};

        virtual ~FileWatcherTest() {};

        virtual void SetUp() {
        };

        virtual void TearDown() {
        };
    };

    /// check basic functions of flare::base::file_watcher
    TEST_F(FileWatcherTest, random_op) {
        srand(time(0));

        flare::base::file_watcher fw;
        EXPECT_EQ (0, fw.init("dummy_file"));

        for (int i = 0; i < 30; ++i) {
            if (rand() % 2) {
                const flare::base::file_watcher::Change ret = fw.check_and_consume();
                switch (ret) {
                    case flare::base::file_watcher::UPDATED:
                        LOG(INFO) << fw.filepath() << " is updated";
                        break;
                    case flare::base::file_watcher::CREATED:
                        LOG(INFO) << fw.filepath() << " is created";
                        break;
                    case flare::base::file_watcher::DELETED:
                        LOG(INFO) << fw.filepath() << " is deleted";
                        break;
                    case flare::base::file_watcher::UNCHANGED:
                        LOG(INFO) << fw.filepath() << " does not change or still not exist";
                        break;
                }
            }

            switch (rand() % 2) {
                case 0:
                    ASSERT_EQ(0, system("touch dummy_file"));
                    LOG(INFO) << "action: touch dummy_file";
                    break;
                case 1:
                    ASSERT_EQ(0, system("rm -f dummy_file"));
                    LOG(INFO) << "action: rm -f dummy_file";
                    break;
                case 2:
                    LOG(INFO) << "action: (nothing)";
                    break;
            }

            usleep(10000);
        }
        ASSERT_EQ(0, system("rm -f dummy_file"));
    }

}  // namespace
