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

// Benchmark http-server by multiple threads.

#include <gflags/gflags.h>
#include <flare/fiber/this_fiber.h>
#include <flare/fiber/internal/fiber.h>
#include "flare/log/logging.h"
#include <flare/rpc/channel.h>
#include <flare/rpc/server.h>
#include <flare/variable/all.h>

DEFINE_string(data, "", "POST this data to the http server");
DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(url, "0.0.0.0:8010/HttpService/Echo", "url of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(dummy_port, -1, "Launch dummy server at this port");
DEFINE_string(protocol, "http", "Client-side protocol");

flare::variable::LatencyRecorder g_latency_recorder("client");

static void* sender(void* arg) {
    flare::rpc::Channel* channel = static_cast<flare::rpc::Channel*>(arg);

    while (!flare::rpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        flare::rpc::Controller cntl;

        cntl.set_timeout_ms(FLAGS_timeout_ms/*milliseconds*/);
        cntl.set_max_retry(FLAGS_max_retry);
        cntl.http_request().uri() = FLAGS_url;
        if (!FLAGS_data.empty()) {
            cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            cntl.request_attachment().append(FLAGS_data);
        }

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
        if (!cntl.Failed()) {
            g_latency_recorder << cntl.latency_us();
        } else {
            FLARE_CHECK(flare::rpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            flare::fiber_sleep_for(100000);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    flare::rpc::Channel channel;
    flare::rpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    
    // Initialize the channel, NULL means using default options. 
    // options, see `flare/rpc/channel.h'.
    if (channel.Init(FLAGS_url.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        FLARE_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    std::vector<fiber_id_t> bids;
    std::vector<pthread_t> pids;
    if (!FLAGS_use_fiber) {
        pids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (pthread_create(&pids[i], NULL, sender, &channel) != 0) {
                FLARE_LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        }
    } else {
        bids.resize(FLAGS_thread_num);
        for (int i = 0; i < FLAGS_thread_num; ++i) {
            if (fiber_start_background(
                    &bids[i], NULL, sender, &channel) != 0) {
                FLARE_LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    if (FLAGS_dummy_port >= 0) {
        flare::rpc::StartDummyServerAt(FLAGS_dummy_port);
    }

    while (!flare::rpc::IsAskedToQuit()) {
        sleep(1);
        FLARE_LOG(INFO) << "Sending " << FLAGS_protocol << " requests at qps="
                  << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    FLARE_LOG(INFO) << "benchmark_http is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }

    return 0;
}
