
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/thread/thread.h"

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <atomic>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "testing/gtest_wrap.h"
#include "flare/thread/latch.h"

TEST(thread, index) {
    EXPECT_EQ(flare::thread::thread_index(), 1);
    flare::thread th([]{
        EXPECT_EQ(flare::thread::thread_index(), 2);
        EXPECT_EQ(flare::thread::current_name(), "#2");
    });
    th.start();
    EXPECT_EQ(th.name(), "#2");
    th.join();

    flare::thread th1([]{
        EXPECT_EQ(flare::thread::thread_index(), 3);
        EXPECT_EQ(flare::thread::current_name(), "th#3");
    });
    th1.set_prefix("th");
    th1.start();
    EXPECT_EQ(th1.name(), "th#3");
    th1.join();
}