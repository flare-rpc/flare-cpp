
#include "flare/bootstrap/flags.h"
#include "flare/bootstrap/bootstrap.h"
#include "gflags/gflags.h"
#include "gtest/gtest.h"

DEFINE_bool(test, true, "");

FLARE_RESET_FLAGS(test, false);

namespace flare::bootstrap {

    TEST(OverrideFlag, All) { EXPECT_FALSE(FLAGS_test); }

}  // namespace flare::init

int main(int argc, char **argv) {
    flare::bootstrap::bootstrap_init(argc, argv);
    return ::RUN_ALL_TESTS();
}