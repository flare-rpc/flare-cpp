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

// A multi-threaded client getting keys from a memcache server constantly.

#include <gflags/gflags.h>
#include <flare/fiber/this_fiber.h>
#include <flare/fiber/internal/fiber.h>
#include "flare/log/logging.h"
#include <flare/base/strings.h>
#include <flare/rpc/channel.h>
#include <flare/rpc/memcache.h>
#include <flare/rpc/policy/couchbase_authenticator.h>

DEFINE_int32(thread_num, 10, "Number of threads to send requests");
DEFINE_bool(use_fiber, false, "Use fiber to send requests");
DEFINE_bool(use_couchbase, false, "Use couchbase.");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:11211", "IP Address of server");
DEFINE_string(bucket_name, "", "Couchbase bucktet name");
DEFINE_string(bucket_password, "", "Couchbase bucket password");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(exptime, 0, "The to-be-got data will be expired after so many seconds");
DEFINE_string(key, "hello", "The key to be get");
DEFINE_string(value, "world", "The value associated with the key");
DEFINE_int32(batch, 1, "Pipelined Operations");

flare::variable::LatencyRecorder g_latency_recorder("client");
flare::variable::Adder<int> g_error_count("client_error_count");
flare::static_atomic<int> g_sender_count = FLARE_STATIC_ATOMIC_INIT(0);

static void* sender(void* arg) {
    google::protobuf::RpcChannel* channel = 
        static_cast<google::protobuf::RpcChannel*>(arg);
    const int base_index = g_sender_count.fetch_add(1, std::memory_order_relaxed);

    std::string value;
    std::vector<std::pair<std::string, std::string> > kvs;
    kvs.resize(FLAGS_batch);
    for (int i = 0; i < FLAGS_batch; ++i) {
        kvs[i].first = flare::string_printf("%s%d", FLAGS_key.c_str(), base_index + i);
        kvs[i].second = flare::string_printf("%s%d", FLAGS_value.c_str(), base_index + i);
    }
    flare::rpc::MemcacheRequest request;
    for (int i = 0; i < FLAGS_batch; ++i) {
        FLARE_CHECK(request.Get(kvs[i].first));
    }
    while (!flare::rpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        flare::rpc::MemcacheResponse response;
        flare::rpc::Controller cntl;

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        channel->CallMethod(NULL, &cntl, &request, &response, NULL);
        const int64_t elp = cntl.latency_us();
        if (!cntl.Failed()) {
            g_latency_recorder << cntl.latency_us();
            for (int i = 0; i < FLAGS_batch; ++i) {
                uint32_t flags;
                if (!response.PopGet(&value, &flags, NULL)) {
                    FLARE_LOG(INFO) << "Fail to GET the key, " << response.LastError();
                    flare::rpc::AskToQuit();
                    return NULL;
                }
                FLARE_CHECK(flags == 0xdeadbeef + base_index + i)
                    << "flags=" << flags;
                FLARE_CHECK(kvs[i].second == value)
                    << "base=" << base_index << " i=" << i << " value=" << value;
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
    if (FLAGS_exptime < 0) {
        FLAGS_exptime = 0;
    }

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    flare::rpc::Channel channel;
    
    // Initialize the channel, NULL means using default options. 
    flare::rpc::ChannelOptions options;
    options.protocol = flare::rpc::PROTOCOL_MEMCACHE;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (FLAGS_use_couchbase && !FLAGS_bucket_name.empty()) {
        flare::rpc::policy::CouchbaseAuthenticator* auth =
            new flare::rpc::policy::CouchbaseAuthenticator(FLAGS_bucket_name,
                                                     FLAGS_bucket_password);
        options.auth = auth;
    }

    if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
        FLARE_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Pipeline #batch * #thread_num SET requests into memcache so that we
    // have keys to get.
    flare::rpc::MemcacheRequest request;
    flare::rpc::MemcacheResponse response;
    flare::rpc::Controller cntl;
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        if (!request.Set(flare::string_printf("%s%d", FLAGS_key.c_str(), i),
                         flare::string_printf("%s%d", FLAGS_value.c_str(), i),
                         0xdeadbeef + i, FLAGS_exptime, 0)) {
            FLARE_LOG(ERROR) << "Fail to SET " << i << "th request";
            return -1;
        }
    }
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        FLARE_LOG(ERROR) << "Fail to access memcache, " << cntl.ErrorText();
        return -1;
    }
    for (int i = 0; i < FLAGS_batch * FLAGS_thread_num; ++i) {
        if (!response.PopSet(NULL)) {
            FLARE_LOG(ERROR) << "Fail to SET memcache, i=" << i
                       << ", " << response.LastError();
            return -1;
        }
    }
    if (FLAGS_exptime > 0) {
        FLARE_LOG(INFO) << "Set " << FLAGS_batch * FLAGS_thread_num
                  << " values, expired after " << FLAGS_exptime << " seconds";
    } else {
        FLARE_LOG(INFO) << "Set " << FLAGS_batch * FLAGS_thread_num
                  << " values, never expired";
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

    while (!flare::rpc::IsAskedToQuit()) {
        sleep(1);
        FLARE_LOG(INFO) << "Accessing memcache server at qps=" << g_latency_recorder.qps(1)
                  << " latency=" << g_latency_recorder.latency(1);
    }

    FLARE_LOG(INFO) << "memcache_client is going to quit";
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        if (!FLAGS_use_fiber) {
            pthread_join(pids[i], NULL);
        } else {
            fiber_join(bids[i], NULL);
        }
    }
    if (options.auth) {
        delete options.auth;
    }

    return 0;
}
