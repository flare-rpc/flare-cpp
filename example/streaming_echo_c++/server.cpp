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

// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include <flare/rpc/server.h>
#include "echo.pb.h"
#include <flare/rpc/stream.h>

DEFINE_bool(send_attachment, true, "Carry attachment along with response");
DEFINE_int32(port, 8001, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");

class StreamReceiver : public flare::rpc::StreamInputHandler {
public:
    virtual int on_received_messages(flare::rpc::StreamId id,
                                     flare::io::cord_buf *const messages[],
                                     size_t size) {
        std::ostringstream os;
        for (size_t i = 0; i < size; ++i) {
            os << "msg[" << i << "]=" << *messages[i];
        }
        LOG(INFO) << "Received from Stream=" << id << ": " << os.str();
        return 0;
    }
    virtual void on_idle_timeout(flare::rpc::StreamId id) {
        LOG(INFO) << "Stream=" << id << " has no data transmission for a while";
    }
    virtual void on_closed(flare::rpc::StreamId id) {
        LOG(INFO) << "Stream=" << id << " is closed";
    }

};

// Your implementation of example::EchoService
class StreamingEchoService : public example::EchoService {
public:
    StreamingEchoService() : _sd(flare::rpc::INVALID_STREAM_ID) {};
    virtual ~StreamingEchoService() {
        flare::rpc::StreamClose(_sd);
    };
    virtual void Echo(google::protobuf::RpcController* controller,
                      const example::EchoRequest* /*request*/,
                      example::EchoResponse* response,
                      google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        flare::rpc::ClosureGuard done_guard(done);

        flare::rpc::Controller* cntl =
            static_cast<flare::rpc::Controller*>(controller);
        flare::rpc::StreamOptions stream_options;
        stream_options.handler = &_receiver;
        if (flare::rpc::StreamAccept(&_sd, *cntl, &stream_options) != 0) {
            cntl->SetFailed("Fail to accept stream");
            return;
        }
        response->set_message("Accepted stream");
    }

private:
    StreamReceiver _receiver;
    flare::rpc::StreamId _sd;
};

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    flare::rpc::Server server;

    // Instance of your service.
    StreamingEchoService echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use flare::rpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          flare::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server. 
    flare::rpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
