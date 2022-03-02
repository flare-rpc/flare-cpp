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



// Date: Sun Jul 13 15:04:18 CST 2014
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "flare/base/gperftools_profiler.h"
#include "flare/base/time.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/server.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/controller.h"
#include "flare/rpc/span.h"
#include "flare/rpc/reloadable_flags.h"
#include "flare/rpc/builtin/version_service.h"
#include "flare/rpc/builtin/health_service.h"
#include "flare/rpc/builtin/list_service.h"
#include "flare/rpc/builtin/status_service.h"
#include "flare/rpc/builtin/threads_service.h"
#include "flare/rpc/builtin/index_service.h"        // IndexService
#include "flare/rpc/builtin/connections_service.h"  // ConnectionsService
#include "flare/rpc/builtin/flags_service.h"        // FlagsService
#include "flare/rpc/builtin/vars_service.h"         // VarsService
#include "flare/rpc/builtin/rpcz_service.h"         // RpczService
#include "flare/rpc/builtin/dir_service.h"          // DirService
#include "flare/rpc/builtin/pprof_service.h"        // PProfService
#include "flare/rpc/builtin/fibers_service.h"     // FibersService
#include "flare/rpc/builtin/token_service.h"          // TokenService
#include "flare/rpc/builtin/sockets_service.h"      // SocketsService
#include "flare/rpc/builtin/common.h"
#include "flare/rpc/builtin/bad_method_service.h"
#include "echo.pb.h"

DEFINE_bool(foo, false, "Flags for UT");
FLARE_RPC_VALIDATE_GFLAG(foo, flare::rpc::PassValidate);

namespace flare::rpc {
    DECLARE_bool(enable_rpcz);
    DECLARE_bool(rpcz_hex_log_id);
    DECLARE_int32(idle_timeout_second);
} // namespace rpc

int main(int argc, char *argv[]) {
    flare::rpc::FLAGS_idle_timeout_second = 0;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class EchoServiceImpl : public test::EchoService {
public:
    EchoServiceImpl() {}

    virtual ~EchoServiceImpl() {}

    virtual void Echo(google::protobuf::RpcController *cntl_base,
                      const test::EchoRequest *req,
                      test::EchoResponse *res,
                      google::protobuf::Closure *done) {
        flare::rpc::Controller *cntl =
                static_cast<flare::rpc::Controller *>(cntl_base);
        flare::rpc::ClosureGuard done_guard(done);
        TRACEPRINTF("MyAnnotation: %ld", cntl->log_id());
        if (req->sleep_us() > 0) {
            flare::fiber_sleep_for(req->sleep_us());
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%" PRIu64, cntl->trace_id());
        res->set_message(buf);
    }
};

class ClosureChecker : public google::protobuf::Closure {
public:
    ClosureChecker() : _count(1) {}

    ~ClosureChecker() { EXPECT_EQ(0, _count); }

    void Run() {
        --_count;
    }

private:
    int _count;
};

void MyVLogSite() {
    VLOG(3) << "This is a VLOG!";
}

void CheckContent(const flare::rpc::Controller &cntl, const char *name) {
    const std::string &content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos) << "name=" << name<<"\n content="<<content;
}

void CheckErrorText(const flare::rpc::Controller &cntl, const char *error) {
    std::size_t pos = cntl.ErrorText().find(error);
    ASSERT_TRUE(pos != std::string::npos) << "error=" << error;
}

void CheckFieldInContent(const flare::rpc::Controller &cntl,
                         const char *name, int32_t expect) {
    const std::string &content = cntl.response_attachment().to_string();
    std::size_t pos = content.find(name);
    ASSERT_TRUE(pos != std::string::npos);

    int32_t val = 0;
    ASSERT_EQ(1, sscanf(content.c_str() + pos + strlen(name), "%d", &val));
    ASSERT_EQ(expect, val) << "name=" << name;
}

void CheckAnnotation(const flare::rpc::Controller &cntl, int64_t expect) {
    const std::string &content = cntl.response_attachment().to_string();
    std::string expect_str;
    flare::string_printf(&expect_str, "MyAnnotation: %" PRId64, expect);
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect;
}

void CheckTraceId(const flare::rpc::Controller &cntl,
                  const std::string &expect_id_str) {
    const std::string &content = cntl.response_attachment().to_string();
    std::string expect_str = std::string(flare::rpc::TRACE_ID_STR) + "=" + expect_id_str;
    std::size_t pos = content.find(expect_str);
    ASSERT_TRUE(pos != std::string::npos) << expect_str;
}

class BuiltinServiceTest : public ::testing::Test {
protected:

    BuiltinServiceTest() {};

    virtual ~BuiltinServiceTest() {};

    virtual void SetUp() { EXPECT_EQ(0, _server.AddBuiltinServices()); }

    virtual void TearDown() { StopAndJoin(); }

    void StopAndJoin() {
        _server.Stop(0);
        _server.Join();
        _server.ClearServices();
    }

    void SetUpController(flare::rpc::Controller *cntl, bool use_html) const {
        cntl->_server = &_server;
        if (use_html) {
            cntl->http_request().SetHeader(
                    flare::rpc::USER_AGENT_STR, "just keep user agent non-empty");
        }
    }

    void TestIndex(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::IndexService service;
        flare::rpc::IndexRequest req;
        flare::rpc::IndexResponse res;
        flare::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
    }

    void TestStatus(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::StatusService service;
        flare::rpc::StatusRequest req;
        flare::rpc::StatusResponse res;
        flare::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        EchoServiceImpl echo_svc;
        ASSERT_EQ(0, _server.AddService(
                &echo_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        ASSERT_EQ(0, _server.RemoveService(&echo_svc));
    }


    void TestConnections(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::ConnectionsService service;
        flare::rpc::ConnectionsRequest req;
        flare::rpc::ConnectionsResponse res;
        flare::rpc::Controller cntl;
        ClosureChecker done;
        SetUpController(&cntl, use_html);
        flare::base::end_point ep;
        ASSERT_EQ(0, str2endpoint("127.0.0.1:9798", &ep));
        ASSERT_EQ(0, _server.Start(ep, NULL));
        int self_port = -1;
        const int cfd = tcp_connect(ep, &self_port);
        ASSERT_GT(cfd, 0);
        char buf[64];
        snprintf(buf, sizeof(buf), "127.0.0.1:%d", self_port);
        usleep(100000);

        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(expect_type, cntl.http_response().content_type());
        CheckContent(cntl, buf);
        CheckFieldInContent(cntl, "channel_connection_count: ", 0);

        close(cfd);
        StopAndJoin();
    }

    void TestBadMethod(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::BadMethodService service;
        flare::rpc::BadMethodResponse res;
        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            flare::rpc::BadMethodRequest req;
            req.set_service_name(
                    flare::rpc::PProfService::descriptor()->full_name());
            service.no_method(&cntl, &req, &res, &done);
            EXPECT_EQ(flare::rpc::ENOMETHOD, cntl.ErrorCode());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckErrorText(cntl, "growth");
        }
    }

    void TestFlags(bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::FlagsService service;
        flare::rpc::FlagsRequest req;
        flare::rpc::FlagsResponse res;
        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "fiber_concurrency");
        }
        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "false");
        }
        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            cntl.http_request().uri()
                    .SetQuery(flare::rpc::SETVALUE_STR, "true");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
        }
        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request()._unresolved_path = "foo";
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            CheckContent(cntl, "true");
        }
    }

    void TestRpcz(bool enable, bool hex, bool use_html) {
        std::string expect_type = (use_html ? "text/html" : "text/plain");
        flare::rpc::RpczService service;
        flare::rpc::RpczRequest req;
        flare::rpc::RpczResponse res;
        if (!enable) {
            {
                ClosureChecker done;
                flare::rpc::Controller cntl;
                SetUpController(&cntl, use_html);
                service.disable(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                EXPECT_FALSE(flare::rpc::FLAGS_enable_rpcz);
            }
            {
                ClosureChecker done;
                flare::rpc::Controller cntl;
                SetUpController(&cntl, use_html);
                service.default_method(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                EXPECT_EQ(expect_type,
                          cntl.http_response().content_type());
                if (!use_html) {
                    CheckContent(cntl, "rpcz is not enabled");
                }
            }
            {
                ClosureChecker done;
                flare::rpc::Controller cntl;
                SetUpController(&cntl, use_html);
                service.stats(&cntl, &req, &res, &done);
                EXPECT_FALSE(cntl.Failed());
                if (!use_html) {
                    CheckContent(cntl, "rpcz is not enabled");
                }
            }
            return;
        }

        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.enable(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            EXPECT_TRUE(flare::rpc::FLAGS_enable_rpcz);
        }

        if (hex) {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.hex_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_TRUE(flare::rpc::FLAGS_rpcz_hex_log_id);
        } else {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.dec_log_id(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            EXPECT_FALSE(flare::rpc::FLAGS_rpcz_hex_log_id);
        }

        ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                        flare::rpc::SERVER_OWNS_SERVICE));
        flare::base::end_point ep;
        ASSERT_EQ(0, str2endpoint("127.0.0.1:9748", &ep));
        ASSERT_EQ(0, _server.Start(ep, NULL));
        flare::rpc::Channel channel;
        ASSERT_EQ(0, channel.Init(ep, NULL));
        test::EchoService_Stub stub(&channel);
        int64_t log_id = 1234567890;
        char querystr_buf[128];
        // Since LevelDB is unstable on jerkins, disable all the assertions here
        {
            // Find by trace_id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            flare::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, NULL);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(flare::rpc::TRACE_ID_STR, echo_res.message());
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckAnnotation(cntl, log_id);
        }

        {
            // Find by latency
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            flare::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_req.set_sleep_us(150000);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, NULL);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(flare::rpc::MIN_LATENCY_STR, "100000");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by request size
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            flare::rpc::Controller echo_cntl;
            std::string request_str(1500, 'a');
            echo_req.set_message(request_str);
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, NULL);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            cntl.http_request().uri()
                    .SetQuery(flare::rpc::MIN_REQUEST_SIZE_STR, "1024");
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            // Find by log id
            test::EchoRequest echo_req;
            test::EchoResponse echo_res;
            flare::rpc::Controller echo_cntl;
            echo_req.set_message("hello");
            echo_cntl.set_log_id(++log_id);
            stub.Echo(&echo_cntl, &echo_req, &echo_res, NULL);
            EXPECT_FALSE(echo_cntl.Failed());

            // Wait for level db to commit span information
            usleep(500000);
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            snprintf(querystr_buf, sizeof(querystr_buf), "%" PRId64, log_id);
            cntl.http_request().uri()
                    .SetQuery(flare::rpc::LOG_ID_STR, querystr_buf);
            service.default_method(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
            EXPECT_EQ(expect_type, cntl.http_response().content_type());
            // CheckTraceId(cntl, echo_res.message());
        }

        {
            ClosureChecker done;
            flare::rpc::Controller cntl;
            SetUpController(&cntl, use_html);
            service.stats(&cntl, &req, &res, &done);
            EXPECT_FALSE(cntl.Failed());
            // CheckContent(cntl, "rpcz.id_db");
            // CheckContent(cntl, "rpcz.time_db");
        }

        StopAndJoin();
    }

private:
    flare::rpc::Server _server;
};

TEST_F(BuiltinServiceTest, index) {
    TestIndex(false);
    TestIndex(true);
}

TEST_F(BuiltinServiceTest, version) {
    const std::string VERSION = "test_version";
    flare::rpc::VersionService service(&_server);
    flare::rpc::VersionRequest req;
    flare::rpc::VersionResponse res;
    flare::rpc::Controller cntl;
    ClosureChecker done;
    _server.set_version(VERSION);
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(VERSION, cntl.response_attachment().to_string());
}

TEST_F(BuiltinServiceTest, health) {
    const std::string HEALTH_STR = "OK";
    flare::rpc::HealthService service;
    flare::rpc::HealthRequest req;
    flare::rpc::HealthResponse res;
    flare::rpc::Controller cntl;
    SetUpController(&cntl, false);
    ClosureChecker done;
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    EXPECT_EQ(HEALTH_STR, cntl.response_attachment().to_string());
}

class MyHealthReporter : public flare::rpc::HealthReporter {
public:
    void GenerateReport(flare::rpc::Controller *cntl,
                        google::protobuf::Closure *done) {
        cntl->response_attachment().append("i'm ok");
        done->Run();
    }
};

TEST_F(BuiltinServiceTest, customized_health) {
    flare::rpc::ServerOptions opt;
    MyHealthReporter hr;
    opt.health_reporter = &hr;
    ASSERT_EQ(0, _server.Start(9798, &opt));
    flare::rpc::HealthRequest req;
    flare::rpc::HealthResponse res;
    flare::rpc::ChannelOptions copt;
    copt.protocol = flare::rpc::PROTOCOL_HTTP;
    flare::rpc::Channel chan;
    ASSERT_EQ(0, chan.Init("127.0.0.1:9798", &copt));
    flare::rpc::Controller cntl;
    cntl.http_request().uri() = "/health";
    chan.CallMethod(NULL, &cntl, &req, &res, NULL);
    EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
    EXPECT_EQ("i'm ok", cntl.response_attachment());
}

TEST_F(BuiltinServiceTest, status) {
    TestStatus(false);
    TestStatus(true);
}

TEST_F(BuiltinServiceTest, list) {
    flare::rpc::ListService service(&_server);
    flare::rpc::ListRequest req;
    flare::rpc::ListResponse res;
    flare::rpc::Controller cntl;
    ClosureChecker done;
    ASSERT_EQ(0, _server.AddService(new EchoServiceImpl(),
                                    flare::rpc::SERVER_OWNS_SERVICE));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    ASSERT_EQ(1, res.service_size());
    EXPECT_EQ(test::EchoService::descriptor()->name(), res.service(0).name());
}

void *sleep_thread(void *) {
    sleep(1);
    return NULL;
}

TEST_F(BuiltinServiceTest, threads) {
    flare::rpc::ThreadsService service;
    flare::rpc::ThreadsRequest req;
    flare::rpc::ThreadsResponse res;
    flare::rpc::Controller cntl;
    ClosureChecker done;
    pthread_t tid;
    ASSERT_EQ(0, pthread_create(&tid, NULL, sleep_thread, NULL));
    service.default_method(&cntl, &req, &res, &done);
    EXPECT_FALSE(cntl.Failed());
    // Doesn't work under gcc 4.8.2
    // CheckContent(cntl, "sleep_thread");
    pthread_join(tid, NULL);
}


TEST_F(BuiltinServiceTest, connections) {
    TestConnections(false);
    TestConnections(true);
}

TEST_F(BuiltinServiceTest, flags) {
    TestFlags(false);
    TestFlags(true);
}

TEST_F(BuiltinServiceTest, bad_method) {
    TestBadMethod(false);
    TestBadMethod(true);
}

TEST_F(BuiltinServiceTest, vars) {
    // Start server to show variables inside
    ASSERT_EQ(0, _server.Start("127.0.0.1:9798", NULL));
    flare::rpc::VarsService service;
    flare::rpc::VarsRequest req;
    flare::rpc::VarsResponse res;
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        flare::variable::Adder<int64_t> myvar;
        myvar.expose("myvar");
        myvar << 9;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckFieldInContent(cntl, "myvar : ", 9);
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "iobuf*";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "iobuf_block_count");
    }
}

TEST_F(BuiltinServiceTest, rpcz) {
    for (int i = 0; i <= 1; ++i) {  // enable rpcz
        for (int j = 0; j <= 1; ++j) {  // hex log id
            for (int k = 0; k <= 1; ++k) {  // use html
                TestRpcz(i, j, k);
            }
        }
    }
}

TEST_F(BuiltinServiceTest, pprof) {
    flare::rpc::PProfService service;
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        cntl.http_request().uri().SetQuery("seconds", "1");
        service.profile(&cntl, NULL, NULL, &done);
        // Just for loading symbols in gperftools/profiler.h
        ProfilerFlush();
        EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
        EXPECT_GT(cntl.response_attachment().length(), 0ul);
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.heap(&cntl, NULL, NULL, &done);
        const int rc = getenv("TCMALLOC_SAMPLE_PARAMETER") != nullptr ? 0 : flare::rpc::ENOMETHOD;
        EXPECT_EQ(rc, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.growth(&cntl, NULL, NULL, &done);
        // linked tcmalloc in UT
        EXPECT_EQ(0, cntl.ErrorCode()) << cntl.ErrorText();
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.symbol(&cntl, NULL, NULL, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "num_symbols");
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.cmdline(&cntl, NULL, NULL, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "rpc_builtin_service_test");
    }
}

TEST_F(BuiltinServiceTest, dir) {
    flare::rpc::DirService service;
    flare::rpc::DirRequest req;
    flare::rpc::DirResponse res;
    {
        // Open root path
        ClosureChecker done;
        flare::rpc::Controller cntl;
        SetUpController(&cntl, true);
        cntl.http_request()._unresolved_path = "";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "tmp");
    }
    {
        // Open a specific file
        ClosureChecker done;
        flare::rpc::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "/usr/include/errno.h";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
#if defined(FLARE_PLATFORM_LINUX)
        CheckContent(cntl, "ERRNO_H");
#elif defined(FLARE_PLATFORM_OSX)
        CheckContent(cntl, "sys/errno.h");
#endif
    }
    {
        // Open a file that doesn't exist
        ClosureChecker done;
        flare::rpc::Controller cntl;
        SetUpController(&cntl, false);
        cntl.http_request()._unresolved_path = "file_not_exist";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "Cannot open");
    }
}

TEST_F(BuiltinServiceTest, token) {
    flare::rpc::TokenService service;
    flare::rpc::TokenRequest req;
    flare::rpc::TokenResponse res;
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /token/<call_id>");
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber_token");
    }
    {
        fiber_token_t id;
        EXPECT_EQ(0, fiber_token_create(&id, NULL, NULL));
        ClosureChecker done;
        flare::rpc::Controller cntl;
        std::string id_string;
        flare::string_printf(&id_string, "%llu", (unsigned long long) id.value);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Status: UNLOCKED");
    }
}

void *dummy_fiber(void *) {
    flare::fiber_sleep_for(1000000);
    return NULL;
}

TEST_F(BuiltinServiceTest, fibers) {
    flare::rpc::FibersService service;
    flare::rpc::FibersRequest req;
    flare::rpc::FibersResponse res;
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /fibers/<fiber_id>");
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a fiber id");
    }
    {
        fiber_id_t th;
        EXPECT_EQ(0, fiber_start_background(&th, NULL, dummy_fiber, NULL));
        ClosureChecker done;
        flare::rpc::Controller cntl;
        std::string id_string;
        flare::string_printf(&id_string, "%llu", (unsigned long long) th);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "stop=0");
    }
}

TEST_F(BuiltinServiceTest, sockets) {
    flare::rpc::SocketsService service;
    flare::rpc::SocketsRequest req;
    flare::rpc::SocketsResponse res;
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "Use /sockets/<SocketId>");
    }
    {
        ClosureChecker done;
        flare::rpc::Controller cntl;
        cntl.http_request()._unresolved_path = "not_valid";
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_TRUE(cntl.Failed());
        CheckErrorText(cntl, "is not a SocketId");
    }
    {
        flare::rpc::SocketId id;
        flare::rpc::SocketOptions options;
        EXPECT_EQ(0, flare::rpc::Socket::Create(options, &id));
        ClosureChecker done;
        flare::rpc::Controller cntl;
        std::string id_string;
        flare::string_printf(&id_string, "%llu", (unsigned long long) id);
        cntl.http_request()._unresolved_path = id_string;
        service.default_method(&cntl, &req, &res, &done);
        EXPECT_FALSE(cntl.Failed());
        CheckContent(cntl, "fd=-1");
    }
}
