//
// Created by liyinbin on 2022/7/1.
//


#include "flare/metrics/gauge.h"
#include "flare/metrics/prometheus_dumper.h"
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>
#include "testing/gtest_wrap.h"

TEST(metrics, gauge) {
    flare::gauge g1("g1","", {{"a","search"}, {"q","qruu"}});
    g1.inc();
    g1.inc(5);
    EXPECT_EQ(g1.get_value(), 6.0);
    auto n = flare::time_now();
    flare::cache_metrics cm;
    g1.collect_metrics(cm);
    auto t = flare::time_now();
    auto str = flare::prometheus_dumper::dump_to_string(cm, &t);
    std::string g1_s = "# HELP g1\n"
                       "# TYPE g1 gauge\n"
                       "g1{a=\"search\",q=\"qruu\"} 6.000000\n";
    std::cout<<g1_s<<std::endl;
    std::cout<<str<<std::endl;
}
