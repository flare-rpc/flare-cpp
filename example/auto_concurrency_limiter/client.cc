// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// A client sending requests to server asynchronously every 1 second.

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include "flare/times/time.h"
#include <flare/rpc/channel.h>
#include <flare/variable/all.h>
#include <flare/fiber/internal/timer_thread.h>
#include <flare/json2pb/json_to_pb.h>

#include <fstream>
#include "cl_test.pb.h"

DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in flare/rpc/options.proto");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(cntl_server, "0.0.0.0:9000", "IP Address of server");
DEFINE_string(echo_server, "0.0.0.0:9001", "IP Address of server");
DEFINE_int32(timeout_ms, 3000, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 0, "Max retries(not including the first RPC)"); 
DEFINE_int32(case_interval, 20, "Intervals for different test cases");
DEFINE_int32(client_qps_change_interval_us, 50000, 
             "The interval for client changes the sending speed");
DEFINE_string(case_file, "", "File path for test_cases");

void DisplayStage(const test::Stage& stage) {
    std::string type;
    switch(stage.type()) {
        case test::FLUCTUATE: 
            type = "Fluctuate";
            break;
        case test::SMOOTH:
            type = "Smooth";
            break;
        default:
            type = "Unknown";
    }
    std::stringstream ss;
    ss 
        << "Stage:[" << stage.lower_bound() << ':' 
        << stage.upper_bound() <<  "]"
        << " , Type:" << type;
    FLARE_LOG(INFO) << ss.str();
}

uint32_t cast_func(void* arg) {
    return *(uint32_t*)arg;
}

std::atomic<uint32_t> g_timeout(0);
std::atomic<uint32_t> g_error(0);
std::atomic<uint32_t> g_succ(0);
flare::variable::PassiveStatus<uint32_t> g_timeout_var(cast_func, &g_timeout);
flare::variable::PassiveStatus<uint32_t> g_error_var(cast_func, &g_error);
flare::variable::PassiveStatus<uint32_t> g_succ_var(cast_func, &g_succ);
flare::variable::LatencyRecorder g_latency_rec;

void LoadCaseSet(test::TestCaseSet* case_set, const std::string& file_path) {
    std::ifstream ifs(file_path.c_str(), std::ios::in);  
    if (!ifs) {
        FLARE_LOG(FATAL) << "Fail to open case set file: " << file_path;
    }
    std::string case_set_json((std::istreambuf_iterator<char>(ifs)),  
                              std::istreambuf_iterator<char>()); 
    std::string err;
    if (!json2pb::JsonToProtoMessage(case_set_json, case_set, &err)) {
        FLARE_LOG(FATAL)
            << "Fail to trans case_set from json to protobuf message: "
            << err;
    }
}

void HandleEchoResponse(
        flare::rpc::Controller* cntl,
        test::NotifyResponse* response) {
    // std::unique_ptr makes sure cntl/response will be deleted before returning.
    std::unique_ptr<flare::rpc::Controller> cntl_guard(cntl);
    std::unique_ptr<test::NotifyResponse> response_guard(response);

    if (cntl->Failed() && cntl->ErrorCode() == flare::rpc::ERPCTIMEDOUT) {
        g_timeout.fetch_add(1, std::memory_order_relaxed);
        FLARE_LOG_EVERY_N(INFO, 1000) << cntl->ErrorText();
    } else if (cntl->Failed()) {
        g_error.fetch_add(1, std::memory_order_relaxed);
        FLARE_LOG_EVERY_N(INFO, 1000) << cntl->ErrorText();
    } else {
        g_succ.fetch_add(1, std::memory_order_relaxed);
        g_latency_rec << cntl->latency_us();
    }

}

void Expose() {
    g_timeout_var.expose_as("cl", "timeout");
    g_error_var.expose_as("cl", "failed");
    g_succ_var.expose_as("cl", "succ");
    g_latency_rec.expose("cl");
}

struct TestCaseContext {
    TestCaseContext(const test::TestCase& tc) 
        : running(true)
        , stage_index(0)
        , test_case(tc)
        , next_stage_sec(test_case.qps_stage_list(0).duration_sec() + 
                         flare::base::gettimeofday_s()) {
        DisplayStage(test_case.qps_stage_list(stage_index));
        Update();
    }

    bool Update() {
        if (flare::base::gettimeofday_s() >= next_stage_sec) {
            ++stage_index;
            if (stage_index < test_case.qps_stage_list_size()) {
                next_stage_sec += test_case.qps_stage_list(stage_index).duration_sec(); 
                DisplayStage(test_case.qps_stage_list(stage_index));
            } else {
                return false;
            }
        }

        int qps = 0;
        const test::Stage& qps_stage = test_case.qps_stage_list(stage_index);
        const int lower_bound = qps_stage.lower_bound();
        const int upper_bound = qps_stage.upper_bound();
        if (qps_stage.type() == test::FLUCTUATE) {
            qps = flare::base::fast_rand_less_than(upper_bound - lower_bound) + lower_bound;
        } else if (qps_stage.type() == test::SMOOTH) {
            qps = lower_bound + (upper_bound - lower_bound) / 
                double(qps_stage.duration_sec()) * (qps_stage.duration_sec() - next_stage_sec
                + flare::base::gettimeofday_s());
        }
        interval_us.store(1.0 / qps * 1000000, std::memory_order_relaxed);
        return true;
    }

    std::atomic<bool> running;
    std::atomic<int64_t> interval_us;
    int stage_index;
    const test::TestCase test_case;
    int next_stage_sec;
};

void RunUpdateTask(void* data) {
    TestCaseContext* context = (TestCaseContext*)data;
    bool should_continue = context->Update();
    if (should_continue) {
        flare::fiber_internal::get_global_timer_thread()->schedule(RunUpdateTask, data,
            flare::base::microseconds_from_now(FLAGS_client_qps_change_interval_us));
    } else {
        context->running.store(false, std::memory_order_release);
    }
}

void RunCase(test::ControlService_Stub &cntl_stub, 
             const test::TestCase& test_case) {
    FLARE_LOG(INFO) << "Running case:`" << test_case.case_name() << '\'';
    flare::rpc::Channel channel;
    flare::rpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_echo_server.c_str(), &options) != 0) {
        FLARE_LOG(FATAL) << "Fail to initialize channel";
    }
    test::EchoService_Stub echo_stub(&channel);

    test::NotifyRequest cntl_req;
    test::NotifyResponse cntl_rsp;
    flare::rpc::Controller cntl;
    cntl_req.set_message("StartCase");
    cntl_stub.Notify(&cntl, &cntl_req, &cntl_rsp, NULL);
    FLARE_CHECK(!cntl.Failed()) << "control failed";

    TestCaseContext context(test_case);
    flare::fiber_internal::get_global_timer_thread()->schedule(RunUpdateTask, &context,
        flare::base::microseconds_from_now(FLAGS_client_qps_change_interval_us));

    while (context.running.load(std::memory_order_acquire)) {
        test::NotifyRequest echo_req;
        echo_req.set_message("hello");
        flare::rpc::Controller* echo_cntl = new flare::rpc::Controller;
        test::NotifyResponse* echo_rsp = new test::NotifyResponse;
        google::protobuf::Closure* done = flare::rpc::NewCallback(
            &HandleEchoResponse, echo_cntl, echo_rsp);
        echo_stub.Echo(echo_cntl, &echo_req, echo_rsp, done);
        ::usleep(context.interval_us.load(std::memory_order_relaxed));
    }

    FLARE_LOG(INFO) << "Waiting to stop case: `" << test_case.case_name() << '\'';
    ::sleep(FLAGS_case_interval);
    cntl.Reset();
    cntl_req.set_message("StopCase");
    cntl_stub.Notify(&cntl, &cntl_req, &cntl_rsp, NULL);
    FLARE_CHECK(!cntl.Failed()) << "control failed";
    FLARE_LOG(INFO) << "Case `" << test_case.case_name() << "' finshed:";
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);
    Expose();

    flare::rpc::Channel channel;
    flare::rpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms;

    if (channel.Init(FLAGS_cntl_server.c_str(), &options) != 0) {
        FLARE_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }
    test::ControlService_Stub cntl_stub(&channel);

    test::TestCaseSet case_set;
    LoadCaseSet(&case_set, FLAGS_case_file);

    flare::rpc::Controller cntl;
    test::NotifyRequest cntl_req;
    test::NotifyResponse cntl_rsp;
    cntl_req.set_message("ResetCaseSet");
    cntl_stub.Notify(&cntl, &cntl_req, &cntl_rsp, NULL);
    FLARE_CHECK(!cntl.Failed()) << "Cntl Failed";
    for (int i = 0; i < case_set.test_case_size(); ++i) {
        RunCase(cntl_stub, case_set.test_case(i));
    }
    FLARE_LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
