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

// A client sending requests to server in batch every 1 second.

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include <flare/rpc/channel.h>
#include <flare/rpc/stream.h>
#include "echo.pb.h"

DEFINE_bool(send_attachment, true, "Carry attachment along with requests");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8001", "IP Address of server");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that 
    // Channel is thread-safe and can be shared by all threads in your program.
    flare::rpc::Channel channel;
        
    // Initialize the channel, NULL means using default options. 
    flare::rpc::ChannelOptions options;
    options.protocol = flare::rpc::PROTOCOL_BAIDU_STD;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(), NULL) != 0) {
        FLARE_LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(&channel);
    flare::rpc::Controller cntl;
    flare::rpc::StreamId stream;
    if (flare::rpc::StreamCreate(&stream, cntl, NULL) != 0) {
        FLARE_LOG(ERROR) << "Fail to create stream";
        return -1;
    }
    FLARE_LOG(INFO) << "Created Stream=" << stream;
    example::EchoRequest request;
    example::EchoResponse response;
    request.set_message("I'm a RPC to connect stream");
    stub.Echo(&cntl, &request, &response, NULL);
    if (cntl.Failed()) {
        FLARE_LOG(ERROR) << "Fail to connect stream, " << cntl.ErrorText();
        return -1;
    }
    
    while (!flare::rpc::IsAskedToQuit()) {
        flare::cord_buf msg1;
        msg1.append("abcdefghijklmnopqrstuvwxyz");
        FLARE_CHECK_EQ(0, flare::rpc::StreamWrite(stream, msg1));
        flare::cord_buf msg2;
        msg2.append("0123456789");
        FLARE_CHECK_EQ(0, flare::rpc::StreamWrite(stream, msg2));
        sleep(1);
    }

    FLARE_CHECK_EQ(0, flare::rpc::StreamClose(stream));
    FLARE_LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
