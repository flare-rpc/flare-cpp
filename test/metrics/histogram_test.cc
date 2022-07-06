
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/metrics/histogram.h"
#include "flare/metrics/prometheus_dumper.h"
#include "flare/base/fast_rand.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include <iostream>
#include "testing/gtest_wrap.h"

TEST(metrics, histogram) {

    auto b = flare::bucket_builder::liner_values(10, 4, 5);
    flare::histogram h1("h1","", b, {{"a","search"}, {"q","qruu"}});
    for(int i =0 ; i < 200; i++) {
        auto v = flare::base::fast_rand_in(0, 25);
        h1.observe(v);
    }
    flare::cache_metrics cm;
    h1.collect_metrics(cm);
    auto t = flare::time_now();
    auto str = flare::prometheus_dumper::dump_to_string(cm, &t);
    std::cout <<str<<std::endl;
}