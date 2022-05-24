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

// A multi-threaded client getting keys from a redis-server constantly.

#include <gflags/gflags.h>
#include <flare/fiber/this_fiber.h>
#include <flare/fiber/internal/fiber.h>
#include "flare/log/logging.h"
#include <flare/base/strings.h>
#include <flare/variable/all.h>
#include <flare/rpc/channel.h>
#include <flare/rpc/server.h>
#include <flare/rpc/redis.h>

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:6379", "IP Address of server");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_string(key, "hello", "The key to be get");
DEFINE_string(value, "world", "The value associated with the key");
DEFINE_int32(batch, 1, "Pipelined Operations");
DEFINE_int32(dummy_port, -1, "port of dummy server(for monitoring)");
DEFINE_int32(backup_request_ms, -1, "Timeout for sending a backup request");

flare::variable::LatencyRecorder g_latency_recorder("client");
flare::variable::Adder<int> g_error_count("client_error_count");

struct SenderArgs {
    int base_index;
    flare::rpc::Channel* redis_channel;
};

static void* sender(void* void_args) {
    SenderArgs* args = (SenderArgs*)void_args;

    std::string value;
    std::vector<std::pair<std::string, std::string> > kvs;
    kvs.resize(FLAGS_batch);
    for (int i = 0; i < FLAGS_batch; ++i) {
        kvs[i].first = flare::string_printf(
            "%s_%04d", FLAGS_key.c_str(), args->base_index + i);
        kvs[i].second = flare::string_printf(
            "%s_%04d", FLAGS_value.c_str(), args->base_index + i);
    }

    flare::rpc::RedisRequest request;
    for (int i = 0; i < FLAGS_batch; ++i) {
        FLARE_CHECK(request.AddCommand("GET %s", kvs[i].first.c_str()));
    }
    while (!flare::rpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        flare::rpc::RedisResponse response;
        flare::rpc::Controller cntl;

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        args->redis_channel->CallMethod(NULL, &cntl, &request, &response, NULL);
        const int64_t elp = cntl.latency_us();
        if (!cntl.Failed()) {
            g_latency_recorder << elp;
            FLARE_CHECK_EQ(response.reply_size(), FLAGS_batch);
            for (int i = 0; i < FLAGS_batch; ++i) {
                FLARE_CHECK_EQ(kvs[i].second.c_str(), response.reply(i).data())
                    << "base=" << args->base_index << " i=" << i;
            }
        } else {
            g_error_count << 1;
            FLARE_CHECK(flare::rpc::IsAskedToQuit() || !FLAGS_dont_fail)
                << "error=" << cntl.ErrorText() << " latency=" << elp;
            // We can't connect to the server, sleep a while. Notice that this
            // is a specific sleeping to prevent this thread from spinning too
            // fast. You should continue the business logic in a production 
            // server rather than sleeping.
            flare::fiber_sleep_for(50000);
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
    
    // Initialize the channel, NULL means using default options. 
    flare::rpc::ChannelOptions options;
    options.protocol = flare::rpc::PROTOCOL_REDIS;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    options.backup_request_ms = FLAGS_backup_request_ms;
    if (channel.Init(FLAGS_server.c_str(), &options) != 0) {
        FLARE_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Pipeline #batch * #thread_num SET requests into redis so that we
    // have keys to get.
    flare::rpc::RedisRequest request;
    flare::rpc::RedisResponse response;
    flare::rpc::Controller cntl;
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        if (!request.AddCommand("SET %s_%04d %s_%04d", 
                    FLAGS_key.c_str(), i,
                    FLAGS_value.c_str(), i)) {
            FLARE_LOG(ERROR) << "Fail to SET " << i << "th request";
            return -1;
        }
    }
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        FLARE_LOG(ERROR) << "Fail to access redis, " << cntl.ErrorText();
        return -1;
    }
    if (FLAGS_batch * FLAGS_thread_num != response.reply_size()) {
        FLARE_LOG(ERROR) << "Fail to set";
        return -1;
    }
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        FLARE_CHECK_EQ("OK", response.reply(i).data());
    }
    FLARE_LOG(INFO) << "Set " << FLAGS_batch * FLAGS_thread_num << " values";

    if (FLAGS_dummy_port >= 0) {
        flare::rpc::StartDummyServerAt(FLAGS_dummy_port);
    }

    std::vector<fiber_id_t> bids;
    std::vector<pthread_t> pids;
    bids.resize(FLAGS_thread_num);
    pids.resize(FLAGS_thread_num);
    std::vector<SenderArgs> args;
    args.resize(FLAGS_thread_num);
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        args[i].base_index = i * FLAGS_batch;
        args[i].redis_channel = &channel;
        if (!FLAGS_use_fiber) {
            if (pthread_create(&pids[i], NULL, sender, &args[i]) != 0) {
                FLARE_LOG(ERROR) << "Fail to create pthread";
                return -1;
            }
        } else {
            if (fiber_start_background(
                    &bids[i], NULL, sender, &args[i]) != 0) {
                FLARE_LOG(ERROR) << "Fail to create fiber";
                return -1;
            }
        }
    }

    while (!flare::rpc::IsAskedToQuit()) {
        sleep(1);
        
        FLARE_LOG(INFO) << "Accessing redis-server at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    FLARE_LOG(INFO) << "redis_client is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }
    return 0;
}
