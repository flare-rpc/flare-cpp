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

// A server to receive EchoRequest and send back EchoResponse asynchronously.

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include <flare/rpc/server.h>
#include "echo.pb.h"

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 8002, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
             "(waiting for client to close connection before server stops)");
DEFINE_int32(max_concurrency, 0, "Limit of request processing in parallel");

std::atomic<int> nsd(0);
struct MySessionLocalData {
    MySessionLocalData() : x(123) {
        nsd.fetch_add(1, std::memory_order_relaxed);
    }
    ~MySessionLocalData() {
        nsd.fetch_sub(1, std::memory_order_relaxed);
    }

    int x;
};

class MySessionLocalDataFactory : public flare::rpc::DataFactory {
public:
    void* CreateData() const {
        return new MySessionLocalData;
    }

    void DestroyData(void* d) const {
        delete static_cast<MySessionLocalData*>(d);
    }
};

std::atomic<int> ntls(0);
struct MyThreadLocalData {
    MyThreadLocalData() : y(0) {
        ntls.fetch_add(1, std::memory_order_relaxed);
    }
    ~MyThreadLocalData() {
        ntls.fetch_sub(1, std::memory_order_relaxed);
    }
    static void deleter(void* d) {
        delete static_cast<MyThreadLocalData*>(d);
    }

    int y;
};

class MyThreadLocalDataFactory : public flare::rpc::DataFactory {
public:
    void* CreateData() const {
        return new MyThreadLocalData;
    }

    void DestroyData(void* d) const {
        MyThreadLocalData::deleter(d);
    }
};

struct AsyncJob {
    MySessionLocalData* expected_session_local_data;
    int expected_session_value;
    flare::rpc::Controller* cntl;
    const example::EchoRequest* request;
    example::EchoResponse* response;
    google::protobuf::Closure* done;

    void run();
    
    void run_and_delete() {
        run();
        delete this;
    }
};

static void* process_thread(void* args) {
    AsyncJob* job = static_cast<AsyncJob*>(args);
    job->run_and_delete();
    return NULL;
}

// Your implementation of example::EchoService
class EchoServiceWithThreadAndSessionLocal : public example::EchoService {
public:
    EchoServiceWithThreadAndSessionLocal() {
        FLARE_CHECK_EQ(0, fiber_key_create(&_tls2_key, MyThreadLocalData::deleter));
    }
    ~EchoServiceWithThreadAndSessionLocal() {
        FLARE_CHECK_EQ(0, fiber_key_delete(_tls2_key));
    };
    void Echo(google::protobuf::RpcController* cntl_base,
              const example::EchoRequest* request,
              example::EchoResponse* response,
              google::protobuf::Closure* done) {
        flare::rpc::ClosureGuard done_guard(done);
        flare::rpc::Controller* cntl =
            static_cast<flare::rpc::Controller*>(cntl_base);

        // Get the session-local data which is created by ServerOptions.session_local_data_factory
        // and reused between different RPC. All session-local data are
        // destroyed upon server destruction.
        MySessionLocalData* sd = static_cast<MySessionLocalData*>(cntl->session_local_data());
        if (sd == NULL) {
            cntl->SetFailed("Require ServerOptions.session_local_data_factory to be"
                            " set with a correctly implemented instance");
            FLARE_LOG(ERROR) << cntl->ErrorText();
            return;
        }
        const int expected_value = sd->x + (((uintptr_t)cntl) & 0xFFFFFFFF);
        sd->x = expected_value;

        // Get the thread-local data which is created by ServerOptions.thread_local_data_factory
        // and reused between different threads. All thread-local data are 
        // destroyed upon server destruction.
        // "tls" is short for "thread local storage".
        MyThreadLocalData* tls =
            static_cast<MyThreadLocalData*>(flare::rpc::thread_local_data());
        if (tls == NULL) {
            cntl->SetFailed("Require ServerOptions.thread_local_data_factory "
                            "to be set with a correctly implemented instance");
            FLARE_LOG(ERROR) << cntl->ErrorText();
            return;
        }
        tls->y = expected_value;

        // You can create fiber-local data for your own.
        // The interfaces are similar with pthread equivalence:
        //   pthread_key_create  -> fiber_key_create
        //   pthread_key_delete  -> fiber_key_delete
        //   pthread_getspecific -> fiber_getspecific
        //   pthread_setspecific -> fiber_setspecific
        MyThreadLocalData* tls2 = 
            static_cast<MyThreadLocalData*>(fiber_getspecific(_tls2_key));
        if (tls2 == NULL) {
            tls2 = new MyThreadLocalData;
            FLARE_CHECK_EQ(0, fiber_setspecific(_tls2_key, tls2));
        }
        tls2->y = expected_value + 1;
        
        // sleep awhile to force context switching.
        flare::fiber_sleep_for(10000);

        // tls is unchanged after context switching.
        FLARE_CHECK_EQ(tls, flare::rpc::thread_local_data());
        FLARE_CHECK_EQ(expected_value, tls->y);

        FLARE_CHECK_EQ(tls2, fiber_getspecific(_tls2_key));
        FLARE_CHECK_EQ(expected_value + 1, tls2->y);

        // Process the request asynchronously.
        AsyncJob* job = new AsyncJob;
        job->expected_session_local_data = sd;
        job->expected_session_value = expected_value;
        job->cntl = cntl;
        job->request = request;
        job->response = response;
        job->done = done;
        fiber_id_t th;
        FLARE_CHECK_EQ(0, fiber_start_background(&th, NULL, process_thread, job));

        // We don't want to call done->Run() here, release the guard.
        done_guard.release();
        
        FLARE_LOG_EVERY_SECOND(INFO) << "ntls=" << ntls.load(std::memory_order_relaxed)
                               << " nsd=" << nsd.load(std::memory_order_relaxed);
    }

private:
    fiber_local_key _tls2_key;
};

void AsyncJob::run() {
    flare::rpc::ClosureGuard done_guard(done);

    // Sleep some time to make sure that Echo() exits.
    flare::fiber_sleep_for(10000);

    // Still the session-local data that we saw in Echo().
    // This is the major difference between session-local data and thread-local
    // data which was already destroyed upon Echo() exit.
    MySessionLocalData* sd = static_cast<MySessionLocalData*>(cntl->session_local_data());
    FLARE_CHECK_EQ(expected_session_local_data, sd);
    FLARE_CHECK_EQ(expected_session_value, sd->x);

    // Echo request and its attachment
    response->set_message(request->message());
    if (FLAGS_echo_attachment) {
        cntl->response_attachment().append(cntl->request_attachment());
    }
}    

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    google::ParseCommandLineFlags(&argc, &argv, true);

    // The factory to create MySessionLocalData. Must be valid when server is running.
    MySessionLocalDataFactory session_local_data_factory;

    MyThreadLocalDataFactory thread_local_data_factory;

    // Generally you only need one Server.
    flare::rpc::Server server;
    // For more options see `flare/rpc/server.h'.
    flare::rpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;
    options.session_local_data_factory = &session_local_data_factory;
    options.thread_local_data_factory = &thread_local_data_factory;

    // Instance of your service.
    EchoServiceWithThreadAndSessionLocal echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use flare::rpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, 
                          flare::rpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        FLARE_LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server. 
    if (server.Start(FLAGS_port, &options) != 0) {
        FLARE_LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    FLARE_CHECK_EQ(ntls, 0);
    FLARE_CHECK_EQ(nsd, 0);
    return 0;
}
