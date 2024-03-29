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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "testing/gtest_wrap.h"
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "flare/times/time.h"
#include "flare/files/sequential_read_file.h"
#include "flare/base/fd_guard.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/acceptor.h"
#include "flare/rpc/server.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/policy/most_common_message.h"
#include "flare/rpc/controller.h"
#include "echo.pb.h"
#include "flare/rpc/policy/http_rpc_protocol.h"
#include "flare/rpc/policy/http2_rpc_protocol.h"
#include "flare/json2pb/pb_to_json.h"
#include "flare/json2pb/json_to_pb.h"
#include "flare/rpc/details/method_status.h"
#include "flare/strings/starts_with.h"
#include "flare/strings/ends_with.h"
#include "flare/fiber/this_fiber.h"

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (google::SetCommandLineOption("socket_max_unwritten_bytes", "2000000").empty()) {
        std::cerr << "Fail to set -socket_max_unwritten_bytes" << std::endl;
        return -1;
    }
    if (google::SetCommandLineOption("flare_crash_on_fatal_log", "true").empty()) {
        std::cerr << "Fail to set -flare_crash_on_fatal_log" << std::endl;
        return -1;
    }
    return RUN_ALL_TESTS();
}

namespace {

    static const std::string EXP_REQUEST = "hello";
    static const std::string EXP_RESPONSE = "world";

    static const std::string MOCK_CREDENTIAL = "mock credential";
    static const std::string MOCK_USER = "mock user";

    class MyAuthenticator : public flare::rpc::Authenticator {
    public:
        MyAuthenticator() {}

        int GenerateCredential(std::string *auth_str) const {
            *auth_str = MOCK_CREDENTIAL;
            return 0;
        }

        int VerifyCredential(const std::string &auth_str,
                             const flare::base::end_point &,
                             flare::rpc::AuthContext *ctx) const {
            EXPECT_EQ(MOCK_CREDENTIAL, auth_str);
            ctx->set_user(MOCK_USER);
            return 0;
        }
    };

    class MyEchoService : public ::test::EchoService {
    public:
        void Echo(::google::protobuf::RpcController *cntl_base,
                  const ::test::EchoRequest *req,
                  ::test::EchoResponse *res,
                  ::google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            const std::string *sleep_ms_str =
                    cntl->http_request().uri().GetQuery("sleep_ms");
            if (sleep_ms_str) {
                flare::fiber_sleep_for(strtol(sleep_ms_str->data(), NULL, 10) * 1000);
            }
            res->set_message(EXP_RESPONSE);
        }
    };

    class HttpTest : public ::testing::Test {
    protected:

        HttpTest() {
            EXPECT_EQ(0, _server.AddBuiltinServices());
            EXPECT_EQ(0, _server.AddService(
                    &_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
            // Hack: Regard `_server' as running
            _server._status = flare::rpc::Server::RUNNING;
            _server._options.auth = &_auth;

            EXPECT_EQ(0, pipe(_pipe_fds));

            flare::rpc::SocketId id;
            flare::rpc::SocketOptions options;
            options.fd = _pipe_fds[1];
            EXPECT_EQ(0, flare::rpc::Socket::Create(options, &id));
            EXPECT_EQ(0, flare::rpc::Socket::Address(id, &_socket));

            flare::rpc::SocketOptions h2_client_options;
            h2_client_options.user = flare::rpc::get_client_side_messenger();
            h2_client_options.fd = _pipe_fds[1];
            EXPECT_EQ(0, flare::rpc::Socket::Create(h2_client_options, &id));
            EXPECT_EQ(0, flare::rpc::Socket::Address(id, &_h2_client_sock));
        };

        virtual ~HttpTest() {};

        virtual void SetUp() {};

        virtual void TearDown() {};

        void VerifyMessage(flare::rpc::InputMessageBase *msg, bool expect) {
            if (msg->_socket == NULL) {
                _socket->ReAddress(&msg->_socket);
            }
            msg->_arg = &_server;
            EXPECT_EQ(expect, flare::rpc::policy::VerifyHttpRequest(msg));
        }

        void ProcessMessage(void (*process)(flare::rpc::InputMessageBase *),
                            flare::rpc::InputMessageBase *msg, bool set_eof) {
            if (msg->_socket == NULL) {
                _socket->ReAddress(&msg->_socket);
            }
            msg->_arg = &_server;
            _socket->PostponeEOF();
            if (set_eof) {
                _socket->SetEOF();
            }
            (*process)(msg);
        }

        flare::rpc::policy::HttpContext *MakePostRequestMessage(const std::string &path) {
            flare::rpc::policy::HttpContext *msg = new flare::rpc::policy::HttpContext(false);
            msg->header().uri().set_path(path);
            msg->header().set_content_type("application/json");
            msg->header().set_method(flare::rpc::HTTP_METHOD_POST);

            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            flare::cord_buf_as_zero_copy_output_stream req_stream(&msg->body());
            EXPECT_TRUE(json2pb::ProtoMessageToJson(req, &req_stream, NULL));
            return msg;
        }

        flare::rpc::policy::HttpContext *MakeGetRequestMessage(const std::string &path) {
            flare::rpc::policy::HttpContext *msg = new flare::rpc::policy::HttpContext(false);
            msg->header().uri().set_path(path);
            msg->header().set_method(flare::rpc::HTTP_METHOD_GET);
            return msg;
        }


        flare::rpc::policy::HttpContext *MakeResponseMessage(int code) {
            flare::rpc::policy::HttpContext *msg = new flare::rpc::policy::HttpContext(false);
            msg->header().set_status_code(code);
            msg->header().set_content_type("application/json");

            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            flare::cord_buf_as_zero_copy_output_stream res_stream(&msg->body());
            EXPECT_TRUE(json2pb::ProtoMessageToJson(res, &res_stream, NULL));
            return msg;
        }

        void CheckResponseCode(bool expect_empty, int expect_code) {
            int bytes_in_pipe = 0;
            ioctl(_pipe_fds[0], FIONREAD, &bytes_in_pipe);
            if (expect_empty) {
                EXPECT_EQ(0, bytes_in_pipe);
                return;
            }

            EXPECT_GT(bytes_in_pipe, 0);
            flare::IOPortal buf;
            EXPECT_EQ((ssize_t) bytes_in_pipe,
                      buf.append_from_file_descriptor(_pipe_fds[0], 1024));
            flare::rpc::ParseResult pr =
                    flare::rpc::policy::ParseHttpMessage(&buf, _socket.get(), false, NULL);
            EXPECT_EQ(flare::rpc::PARSE_OK, pr.error());
            flare::rpc::policy::HttpContext *msg =
                    static_cast<flare::rpc::policy::HttpContext *>(pr.message());

            EXPECT_EQ(expect_code, msg->header().status_code());
            msg->Destroy();
        }

        void MakeH2EchoRequestBuf(flare::cord_buf *out, flare::rpc::Controller *cntl, int *h2_stream_id) {
            flare::cord_buf request_buf;
            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            cntl->http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            flare::rpc::policy::SerializeHttpRequest(&request_buf, cntl, &req);
            ASSERT_FALSE(cntl->Failed());
            flare::rpc::policy::H2UnsentRequest *h2_req = flare::rpc::policy::H2UnsentRequest::New(cntl);
            cntl->_current_call.stream_user_data = h2_req;
            flare::rpc::SocketMessage *socket_message = NULL;
            flare::rpc::policy::PackH2Request(NULL, &socket_message, cntl->call_id().value,
                                              NULL, cntl, request_buf, NULL);
            flare::result_status st = socket_message->AppendAndDestroySelf(out, _h2_client_sock.get());
            ASSERT_TRUE(st.is_ok());
            *h2_stream_id = h2_req->_stream_id;
        }

        void MakeH2EchoResponseBuf(flare::cord_buf *out, int h2_stream_id) {
            flare::rpc::Controller cntl;
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            cntl.http_request().set_content_type("application/proto");
            {
                flare::cord_buf_as_zero_copy_output_stream wrapper(&cntl.response_attachment());
                EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
            }
            flare::rpc::policy::H2UnsentResponse *h2_res = flare::rpc::policy::H2UnsentResponse::New(&cntl,
                                                                                                     h2_stream_id,
                                                                                                     false /*is grpc*/);
            flare::result_status st = h2_res->AppendAndDestroySelf(out, _h2_client_sock.get());
            ASSERT_TRUE(st);
        }

        int _pipe_fds[2];
        flare::rpc::SocketUniquePtr _socket;
        flare::rpc::SocketUniquePtr _h2_client_sock;
        flare::rpc::Server _server;

        MyEchoService _svc;
        MyAuthenticator _auth;
    };

    TEST_F(HttpTest, indenting_ostream) {
        std::ostringstream os1;
        flare::rpc::IndentingOStream is1(os1, 2);
        flare::rpc::IndentingOStream is2(is1, 2);
        os1 << "begin1\nhello" << std::endl << "world\nend1" << std::endl;
        is1 << "begin2\nhello" << std::endl << "world\nend2" << std::endl;
        is2 << "begin3\nhello" << std::endl << "world\nend3" << std::endl;
        ASSERT_EQ(
                "begin1\nhello\nworld\nend1\nbegin2\n  hello\n  world\n  end2\n"
                "  begin3\n    hello\n    world\n    end3\n",
                os1.str());
    }

    TEST_F(HttpTest, parse_http_address) {
        const std::string EXP_HOSTNAME = "www.baidu.com:9876";
        flare::base::end_point EXP_ENDPOINT;
        {
            std::string url = "https://" + EXP_HOSTNAME;
            EXPECT_TRUE(flare::rpc::policy::ParseHttpServerAddress(&EXP_ENDPOINT, url.c_str()));
        }
        {
            flare::base::end_point ep;
            std::string url = "http://" +
                              std::string(endpoint2str(EXP_ENDPOINT).c_str());
            EXPECT_TRUE(flare::rpc::policy::ParseHttpServerAddress(&ep, url.c_str()));
            EXPECT_EQ(EXP_ENDPOINT, ep);
        }
        {
            flare::base::end_point ep;
            std::string url = "https://" +
                              std::string(flare::base::ip2str(EXP_ENDPOINT.ip).c_str());
            EXPECT_TRUE(flare::rpc::policy::ParseHttpServerAddress(&ep, url.c_str()));
            EXPECT_EQ(EXP_ENDPOINT.ip, ep.ip);
            EXPECT_EQ(443, ep.port);
        }
        {
            flare::base::end_point ep;
            EXPECT_FALSE(flare::rpc::policy::ParseHttpServerAddress(&ep, "invalid_url"));
        }
        {
            flare::base::end_point ep;
            EXPECT_FALSE(flare::rpc::policy::ParseHttpServerAddress(
                    &ep, "https://no.such.machine:9090"));
        }
    }

    TEST_F(HttpTest, verify_request) {
        {
            flare::rpc::policy::HttpContext *msg =
                    MakePostRequestMessage("/EchoService/Echo");
            VerifyMessage(msg, false);
            msg->Destroy();
        }
        {
            flare::rpc::policy::HttpContext *msg = MakeGetRequestMessage("/status");
            VerifyMessage(msg, true);
            msg->Destroy();
        }
        {
            flare::rpc::policy::HttpContext *msg =
                    MakePostRequestMessage("/EchoService/Echo");
            _socket->SetFailed();
            VerifyMessage(msg, false);
            msg->Destroy();
        }
    }

    TEST_F(HttpTest, process_request_failed_socket) {
        flare::rpc::policy::HttpContext *msg = MakePostRequestMessage("/EchoService/Echo");
        _socket->SetFailed();
        ProcessMessage(flare::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(0ll, _server._nerror_var.get_value());
        CheckResponseCode(true, 0);
    }

    TEST_F(HttpTest, reject_get_to_pb_services_with_required_fields) {
        flare::rpc::policy::HttpContext *msg = MakeGetRequestMessage("/EchoService/Echo");
        _server._status = flare::rpc::Server::RUNNING;
        ProcessMessage(flare::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(0ll, _server._nerror_var.get_value());
        const flare::rpc::Server::MethodProperty *mp =
                _server.FindMethodPropertyByFullName("test.EchoService.Echo");
        ASSERT_TRUE(mp);
        ASSERT_TRUE(mp->status);
        ASSERT_EQ(1ll, mp->status->_nerror_var.get_value());
        CheckResponseCode(false, flare::rpc::HTTP_STATUS_BAD_REQUEST);
    }

    TEST_F(HttpTest, process_request_logoff) {
        flare::rpc::policy::HttpContext *msg = MakePostRequestMessage("/EchoService/Echo");
        _server._status = flare::rpc::Server::READY;
        ProcessMessage(flare::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(1ll, _server._nerror_var.get_value());
        CheckResponseCode(false, flare::rpc::HTTP_STATUS_SERVICE_UNAVAILABLE);
    }

    TEST_F(HttpTest, process_request_wrong_method) {
        flare::rpc::policy::HttpContext *msg = MakePostRequestMessage("/NO_SUCH_METHOD");
        ProcessMessage(flare::rpc::policy::ProcessHttpRequest, msg, false);
        ASSERT_EQ(1ll, _server._nerror_var.get_value());
        CheckResponseCode(false, flare::rpc::HTTP_STATUS_NOT_FOUND);
    }

    TEST_F(HttpTest, process_response_after_eof) {
        test::EchoResponse res;
        flare::rpc::Controller cntl;
        cntl._response = &res;
        flare::rpc::policy::HttpContext *msg =
                MakeResponseMessage(flare::rpc::HTTP_STATUS_OK);
        _socket->set_correlation_id(cntl.call_id().value);
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, msg, true);
        ASSERT_EQ(EXP_RESPONSE, res.message());
        ASSERT_TRUE(_socket->Failed());
    }

    TEST_F(HttpTest, process_response_error_code) {
        {
            flare::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            flare::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(flare::rpc::HTTP_STATUS_CONTINUE);
            ProcessMessage(flare::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(flare::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(flare::rpc::HTTP_STATUS_CONTINUE, cntl.http_response().status_code());
        }
        {
            flare::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            flare::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(flare::rpc::HTTP_STATUS_TEMPORARY_REDIRECT);
            ProcessMessage(flare::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(flare::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(flare::rpc::HTTP_STATUS_TEMPORARY_REDIRECT,
                      cntl.http_response().status_code());
        }
        {
            flare::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            flare::rpc::policy::HttpContext *msg =
                    MakeResponseMessage(flare::rpc::HTTP_STATUS_BAD_REQUEST);
            ProcessMessage(flare::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(flare::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(flare::rpc::HTTP_STATUS_BAD_REQUEST,
                      cntl.http_response().status_code());
        }
        {
            flare::rpc::Controller cntl;
            _socket->set_correlation_id(cntl.call_id().value);
            flare::rpc::policy::HttpContext *msg = MakeResponseMessage(12345);
            ProcessMessage(flare::rpc::policy::ProcessHttpResponse, msg, false);
            ASSERT_EQ(flare::rpc::EHTTP, cntl.ErrorCode());
            ASSERT_EQ(12345, cntl.http_response().status_code());
        }
    }

    TEST_F(HttpTest, complete_flow) {
        flare::cord_buf request_buf;
        flare::cord_buf total_buf;
        flare::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        cntl._response = &res;
        cntl._connection_type = flare::rpc::CONNECTION_TYPE_SHORT;
        cntl._method = test::EchoService::descriptor()->method(0);
        ASSERT_EQ(0, flare::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

        // Send request
        req.set_message(EXP_REQUEST);
        flare::rpc::policy::SerializeHttpRequest(&request_buf, &cntl, &req);
        ASSERT_FALSE(cntl.Failed());
        flare::rpc::policy::PackHttpRequest(
                &total_buf, NULL, cntl.call_id().value,
                cntl._method, &cntl, request_buf, &_auth);
        ASSERT_FALSE(cntl.Failed());

        // Verify and handle request
        flare::rpc::ParseResult req_pr =
                flare::rpc::policy::ParseHttpMessage(&total_buf, _socket.get(), false, NULL);
        ASSERT_EQ(flare::rpc::PARSE_OK, req_pr.error());
        flare::rpc::InputMessageBase *req_msg = req_pr.message();
        VerifyMessage(req_msg, true);
        ProcessMessage(flare::rpc::policy::ProcessHttpRequest, req_msg, false);

        // Read response from pipe
        flare::IOPortal response_buf;
        response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseHttpMessage(&response_buf, _socket.get(), false, NULL);
        ASSERT_EQ(flare::rpc::PARSE_OK, res_pr.error());
        flare::rpc::InputMessageBase *res_msg = res_pr.message();
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_msg, false);

        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(EXP_RESPONSE, res.message());
    }

    TEST_F(HttpTest, chunked_uploading) {
        const int port = 8923;
        flare::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));

        // Send request via curl using chunked encoding
        const std::string req = "{\"message\":\"hello\"}";
        const std::string res_fname = "curl.out";
        std::string cmd;
        flare::string_printf(&cmd, "curl -X POST -d '%s' -H 'Transfer-Encoding:chunked' "
                                   "-H 'Content-Type:application/json' -o %s "
                                   "http://localhost:%d/EchoService/Echo",
                             req.c_str(), res_fname.c_str(), port);
        ASSERT_EQ(0, system(cmd.c_str()));

        // Check response
        const std::string exp_res = "{\"message\":\"world\"}";
        flare::sequential_read_file file;
        file.open(res_fname);
        std::string content;
        ASSERT_TRUE(file.read(&content));
        EXPECT_EQ(exp_res, content);
    }

    enum DonePlace {
        DONE_BEFORE_CREATE_PA = 0,
        DONE_AFTER_CREATE_PA_BEFORE_DESTROY_PA,
        DONE_AFTER_DESTROY_PA,
    };
// For writing into PA.
    const char PA_DATA[] = "abcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_=-+";
    const size_t PA_DATA_LEN = sizeof(PA_DATA) - 1/*not count the ending zero*/;

    static void CopyPAPrefixedWithSeqNo(char *buf, uint64_t seq_no) {
        memcpy(buf, PA_DATA, PA_DATA_LEN);
        *(uint64_t *) buf = seq_no;
    }

    class DownloadServiceImpl : public ::test::DownloadService {
    public:
        DownloadServiceImpl(DonePlace done_place = DONE_BEFORE_CREATE_PA,
                            size_t num_repeat = 1)
                : _done_place(done_place), _nrep(num_repeat), _nwritten(0), _ever_full(false), _last_errno(0) {}

        void Download(::google::protobuf::RpcController *cntl_base,
                      const ::test::HttpRequest *,
                      ::test::HttpResponse *,
                      ::google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            cntl->http_response().set_content_type("text/plain");
            flare::rpc::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                                                ? flare::rpc::FORCE_STOP : flare::rpc::WAIT_FOR_STOP);
            flare::container::intrusive_ptr<flare::rpc::ProgressiveAttachment> pa
                    = cntl->CreateProgressiveAttachment(stop_style);
            if (pa == NULL) {
                cntl->SetFailed("The socket was just failed");
                return;
            }
            if (_done_place == DONE_BEFORE_CREATE_PA) {
                done_guard.reset(NULL);
            }
            ASSERT_GT(PA_DATA_LEN, 8u);  // long enough to hold a 64-bit decimal.
            char buf[PA_DATA_LEN];
            for (size_t c = 0; c < _nrep;) {
                CopyPAPrefixedWithSeqNo(buf, c);
                if (pa->Write(buf, sizeof(buf)) != 0) {
                    if (errno == flare::rpc::EOVERCROWDED) {
                        FLARE_LOG_EVERY_SECOND(INFO) << "full pa=" << pa.get();
                        _ever_full = true;
                        flare::fiber_sleep_for(10000);
                        continue;
                    } else {
                        _last_errno = errno;
                        break;
                    }
                } else {
                    _nwritten += PA_DATA_LEN;
                }
                ++c;
            }
            if (_done_place == DONE_AFTER_CREATE_PA_BEFORE_DESTROY_PA) {
                done_guard.reset(NULL);
            }
            FLARE_LOG(INFO) << "Destroy pa=" << pa.get();
            pa.reset(NULL);
            if (_done_place == DONE_AFTER_DESTROY_PA) {
                done_guard.reset(NULL);
            }
        }

        void DownloadFailed(::google::protobuf::RpcController *cntl_base,
                            const ::test::HttpRequest *,
                            ::test::HttpResponse *,
                            ::google::protobuf::Closure *done) {
            flare::rpc::ClosureGuard done_guard(done);
            flare::rpc::Controller *cntl =
                    static_cast<flare::rpc::Controller *>(cntl_base);
            cntl->http_response().set_content_type("text/plain");
            flare::rpc::StopStyle stop_style = (_nrep == std::numeric_limits<size_t>::max()
                                                ? flare::rpc::FORCE_STOP : flare::rpc::WAIT_FOR_STOP);
            flare::container::intrusive_ptr<flare::rpc::ProgressiveAttachment> pa
                    = cntl->CreateProgressiveAttachment(stop_style);
            if (pa == NULL) {
                cntl->SetFailed("The socket was just failed");
                return;
            }
            char buf[PA_DATA_LEN];
            while (true) {
                if (pa->Write(buf, sizeof(buf)) != 0) {
                    if (errno == flare::rpc::EOVERCROWDED) {
                        FLARE_LOG_EVERY_SECOND(INFO) << "full pa=" << pa.get();
                        flare::fiber_sleep_for(10000);
                        continue;
                    } else {
                        _last_errno = errno;
                        break;
                    }
                }
                break;
            }
            // The remote client will not receive the data written to the
            // progressive attachment when the controller failed.
            cntl->SetFailed("Intentionally set controller failed");
            done_guard.reset(NULL);

            // Return value of Write after controller has failed should
            // be less than zero.
            FLARE_CHECK_LT(pa->Write(buf, sizeof(buf)), 0);
            FLARE_CHECK_EQ(errno, ECANCELED);
        }

        void set_done_place(DonePlace done_place) { _done_place = done_place; }

        size_t written_bytes() const { return _nwritten; }

        bool ever_full() const { return _ever_full; }

        int last_errno() const { return _last_errno; }

    private:
        DonePlace _done_place;
        size_t _nrep;
        size_t _nwritten;
        bool _ever_full;
        int _last_errno;
    };

    TEST_F(HttpTest, read_chunked_response_normally) {
        const int port = 8923;
        flare::rpc::Server server;
        DownloadServiceImpl svc;
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));

        for (int i = 0; i < 3; ++i) {
            svc.set_done_place((DonePlace) i);
            flare::rpc::Channel channel;
            flare::rpc::ChannelOptions options;
            options.protocol = flare::rpc::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
            flare::rpc::Controller cntl;
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();

            std::string expected(PA_DATA_LEN, 0);
            CopyPAPrefixedWithSeqNo(&expected[0], 0);
            ASSERT_EQ(expected, cntl.response_attachment());
        }
    }

    TEST_F(HttpTest, read_failed_chunked_response) {
        const int port = 8923;
        flare::rpc::Server server;
        DownloadServiceImpl svc;
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));

        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = flare::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));

        flare::rpc::Controller cntl;
        cntl.http_request().uri() = "/DownloadService/DownloadFailed";
        cntl.response_will_be_read_progressively();
        channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
        EXPECT_TRUE(cntl.response_attachment().empty());
        ASSERT_TRUE(cntl.Failed());
        ASSERT_NE(cntl.ErrorText().find("HTTP/1.1 500 Internal Server Error"),
                  std::string::npos) << cntl.ErrorText();
        ASSERT_NE(cntl.ErrorText().find("Intentionally set controller failed"),
                  std::string::npos) << cntl.ErrorText();
        ASSERT_EQ(0, svc.last_errno());
    }

    class ReadBody : public flare::rpc::ProgressiveReader,
                     public flare::rpc::SharedObject {
    public:
        ReadBody()
                : _nread(0), _ncount(0), _destroyed(false) {
            flare::container::intrusive_ptr<ReadBody>(this).detach(); // ref
        }

        flare::result_status OnReadOnePart(const void *data, size_t length) {
            _nread += length;
            while (length > 0) {
                size_t nappend = std::min(_buf.size() + length, PA_DATA_LEN) - _buf.size();
                _buf.append((const char *) data, nappend);
                data = (const char *) data + nappend;
                length -= nappend;
                if (_buf.size() >= PA_DATA_LEN) {
                    EXPECT_EQ(PA_DATA_LEN, _buf.size());
                    char expected[PA_DATA_LEN];
                    CopyPAPrefixedWithSeqNo(expected, _ncount++);
                    EXPECT_EQ(0, memcmp(expected, _buf.data(), PA_DATA_LEN))
                                        << "ncount=" << _ncount;
                    _buf.clear();
                }
            }
            return flare::result_status::success();
        }

        void OnEndOfMessage(const flare::result_status &st) {
            flare::container::intrusive_ptr<ReadBody>(this, false); // deref
            ASSERT_LT(_buf.size(), PA_DATA_LEN);
            ASSERT_EQ(0, memcmp(_buf.data(), PA_DATA, _buf.size()));
            _destroyed = true;
            _destroying_st = st;
            FLARE_LOG(INFO) << "Destroy ReadBody=" << this << ", " << st;
        }

        bool destroyed() const { return _destroyed; }

        const flare::result_status &destroying_status() const { return _destroying_st; }

        size_t read_bytes() const { return _nread; }

    private:
        std::string _buf;
        size_t _nread;
        size_t _ncount;
        bool _destroyed;
        flare::result_status _destroying_st;
    };

    static const int GENERAL_DELAY_US = 300000; // 0.3s

    TEST_F(HttpTest, read_long_body_progressively) {
        flare::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            flare::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, NULL));
            {
                flare::rpc::Channel channel;
                flare::rpc::ChannelOptions options;
                options.protocol = flare::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
                {
                    flare::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                    size_t last_read = 0;
                    for (size_t i = 0; i < 3; ++i) {
                        sleep(1);
                        size_t current_read = reader->read_bytes();
                        FLARE_LOG(INFO) << "read=" << current_read - last_read
                                        << " total=" << current_read;
                        last_read = current_read;
                    }
                    // Read something in past N seconds.
                    ASSERT_GT(last_read, (size_t) 100000);
                }
                // the socket still holds a ref.
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            // even if the main socket is recycled, the pooled socket for
            // receiving data is not affected.
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, read_short_body_progressively) {
        flare::container::intrusive_ptr<ReadBody> reader;
        const int port = 8923;
        flare::rpc::Server server;
        const int NREP = 10000;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA, NREP);
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        {
            flare::rpc::Channel channel;
            flare::rpc::ChannelOptions options;
            options.protocol = flare::rpc::PROTOCOL_HTTP;
            ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
            {
                flare::rpc::Controller cntl;
                cntl.response_will_be_read_progressively();
                cntl.http_request().uri() = "/DownloadService/Download";
                channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                ASSERT_TRUE(cntl.response_attachment().empty());
                reader.reset(new ReadBody);
                cntl.ReadProgressiveAttachmentBy(reader.get());
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    FLARE_LOG(INFO) << "read=" << current_read - last_read
                                    << " total=" << current_read;
                    last_read = current_read;
                }
                ASSERT_EQ(NREP * PA_DATA_LEN, svc.written_bytes());
                ASSERT_EQ(NREP * PA_DATA_LEN, last_read);
            }
            ASSERT_TRUE(reader->destroyed());
            ASSERT_EQ(0, reader->destroying_status().error_code());
        }
    }

    TEST_F(HttpTest, read_progressively_after_cntl_destroys) {
        flare::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            flare::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, NULL));
            {
                flare::rpc::Channel channel;
                flare::rpc::ChannelOptions options;
                options.protocol = flare::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
                {
                    flare::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                }
                size_t last_read = 0;
                for (size_t i = 0; i < 3; ++i) {
                    sleep(1);
                    size_t current_read = reader->read_bytes();
                    FLARE_LOG(INFO) << "read=" << current_read - last_read
                                    << " total=" << current_read;
                    last_read = current_read;
                }
                // Read something in past N seconds.
                ASSERT_GT(last_read, (size_t) 100000);
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, read_progressively_after_long_delay) {
        flare::container::intrusive_ptr<ReadBody> reader;
        {
            const int port = 8923;
            flare::rpc::Server server;
            DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                    std::numeric_limits<size_t>::max());
            EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
            EXPECT_EQ(0, server.Start(port, NULL));
            {
                flare::rpc::Channel channel;
                flare::rpc::ChannelOptions options;
                options.protocol = flare::rpc::PROTOCOL_HTTP;
                ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
                {
                    flare::rpc::Controller cntl;
                    cntl.response_will_be_read_progressively();
                    cntl.http_request().uri() = "/DownloadService/Download";
                    channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
                    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
                    ASSERT_TRUE(cntl.response_attachment().empty());
                    FLARE_LOG(INFO) << "Sleep 3 seconds to make PA at server-side full";
                    sleep(3);
                    EXPECT_TRUE(svc.ever_full());
                    ASSERT_EQ(0, svc.last_errno());
                    reader.reset(new ReadBody);
                    cntl.ReadProgressiveAttachmentBy(reader.get());
                    size_t last_read = 0;
                    for (size_t i = 0; i < 3; ++i) {
                        sleep(1);
                        size_t current_read = reader->read_bytes();
                        FLARE_LOG(INFO) << "read=" << current_read - last_read
                                        << " total=" << current_read;
                        last_read = current_read;
                    }
                    // Read something in past N seconds.
                    ASSERT_GT(last_read, (size_t) 100000);
                }
                ASSERT_FALSE(reader->destroyed());
            }
            // Wait for recycling of the main socket.
            usleep(GENERAL_DELAY_US);
            ASSERT_FALSE(reader->destroyed());
        }
        // Wait for close of the connection due to server's stopping.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, skip_progressive_reading) {
        const int port = 8923;
        flare::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = flare::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
        {
            flare::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
        }
        const size_t old_written_bytes = svc.written_bytes();
        FLARE_LOG(INFO) << "Sleep 3 seconds after destroy of Controller";
        sleep(3);
        const size_t new_written_bytes = svc.written_bytes();
        ASSERT_EQ(0, svc.last_errno());
        FLARE_LOG(INFO) << "Server still wrote " << new_written_bytes - old_written_bytes;
        // The server side still wrote things.
        ASSERT_GT(new_written_bytes - old_written_bytes, (size_t) 100000);
    }

    class AlwaysFailRead : public flare::rpc::ProgressiveReader {
    public:
        // @ProgressiveReader
        flare::result_status OnReadOnePart(const void * /*data*/, size_t /*length*/) {
            return flare::result_status(-1, "intended fail at {}:{}", __FILE__, __LINE__);
        }

        void OnEndOfMessage(const flare::result_status &st) {
            FLARE_LOG(INFO) << "Destroy " << this << ": " << st;
            delete this;
        }
    };

    TEST_F(HttpTest, failed_on_read_one_part) {
        const int port = 8923;
        flare::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = flare::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
        {
            flare::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
            cntl.ReadProgressiveAttachmentBy(new AlwaysFailRead);
        }
        FLARE_LOG(INFO) << "Sleep 1 second";
        sleep(1);
        ASSERT_NE(0, svc.last_errno());
    }

    TEST_F(HttpTest, broken_socket_stops_progressive_reading) {
        flare::container::intrusive_ptr<ReadBody> reader;
        const int port = 8923;
        flare::rpc::Server server;
        DownloadServiceImpl svc(DONE_BEFORE_CREATE_PA,
                                std::numeric_limits<size_t>::max());
        EXPECT_EQ(0, server.AddService(&svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));

        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = flare::rpc::PROTOCOL_HTTP;
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));
        {
            flare::rpc::Controller cntl;
            cntl.response_will_be_read_progressively();
            cntl.http_request().uri() = "/DownloadService/Download";
            channel.CallMethod(NULL, &cntl, NULL, NULL, NULL);
            ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
            ASSERT_TRUE(cntl.response_attachment().empty());
            reader.reset(new ReadBody);
            cntl.ReadProgressiveAttachmentBy(reader.get());
            size_t last_read = 0;
            for (size_t i = 0; i < 3; ++i) {
                sleep(1);
                size_t current_read = reader->read_bytes();
                FLARE_LOG(INFO) << "read=" << current_read - last_read
                                << " total=" << current_read;
                last_read = current_read;
            }
            // Read something in past N seconds.
            ASSERT_GT(last_read, (size_t) 100000);
        }
        // the socket still holds a ref.
        ASSERT_FALSE(reader->destroyed());
        FLARE_LOG(INFO) << "Stopping the server";
        server.Stop(0);
        server.Join();

        // Wait for error reporting from the socket.
        usleep(GENERAL_DELAY_US);
        ASSERT_TRUE(reader->destroyed());
        ASSERT_EQ(ECONNRESET, reader->destroying_status().error_code());
    }

    TEST_F(HttpTest, http2_sanity) {
        const int port = 8923;
        flare::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));

        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = "h2";
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));

        // Check that the first request with size larger than the default window can
        // be sent out, when remote settings are not received.
        flare::rpc::Controller cntl;
        test::EchoRequest big_req;
        test::EchoResponse res;
        std::string message(2 * 1024 * 1024 /* 2M */, 'x');
        big_req.set_message(message);
        cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        channel.CallMethod(NULL, &cntl, &big_req, &res, NULL);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());

        // socket replacement when streamId runs out, the initial streamId is a special
        // value set in ctor of H2Context so that the number 15000 is enough to run out
        // of stream.
        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        for (int i = 0; i < 15000; ++i) {
            flare::rpc::Controller cntl;
            cntl.http_request().set_content_type("application/json");
            cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(NULL, &cntl, &req, &res, NULL);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
        }

        // check connection window size
        flare::rpc::SocketUniquePtr main_ptr;
        flare::rpc::SocketUniquePtr agent_ptr;
        EXPECT_EQ(flare::rpc::Socket::Address(channel._server_id, &main_ptr), 0);
        EXPECT_EQ(main_ptr->GetAgentSocket(&agent_ptr, NULL), 0);
        flare::rpc::policy::H2Context *ctx = static_cast<flare::rpc::policy::H2Context *>(agent_ptr->parsing_context());
        ASSERT_GT(ctx->_remote_window_left.load(std::memory_order_relaxed),
                  flare::rpc::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / 2);
    }

    TEST_F(HttpTest, http2_ping) {
        // This test injects PING frames before and after header and data.
        flare::rpc::Controller cntl;

        // Prepare request
        flare::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        flare::cord_buf res_out;
        char pingbuf[9 /*FRAME_HEAD_SIZE*/ + 8 /*Opaque Data*/];
        flare::rpc::policy::SerializeFrameHead(pingbuf, 8, flare::rpc::policy::H2_FRAME_PING, 0, 0);
        // insert ping before header and data
        res_out.append(pingbuf, sizeof(pingbuf));
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // insert ping after header and data
        res_out.append(pingbuf, sizeof(pingbuf));
        // parse response
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());
    }

    inline void SaveUint32(void *out, uint32_t v) {
        uint8_t *p = (uint8_t *) out;
        p[0] = (v >> 24) & 0xFF;
        p[1] = (v >> 16) & 0xFF;
        p[2] = (v >> 8) & 0xFF;
        p[3] = v & 0xFF;
    }

    TEST_F(HttpTest, http2_rst_before_header) {
        flare::rpc::Controller cntl;
        // Prepare request
        flare::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        flare::cord_buf res_out;
        char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
        flare::rpc::policy::SerializeFrameHead(rstbuf, 4, flare::rpc::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
        SaveUint32(rstbuf + 9, flare::rpc::H2_INTERNAL_ERROR);
        res_out.append(rstbuf, sizeof(rstbuf));
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // parse response
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_TRUE(cntl.Failed());
        ASSERT_TRUE(cntl.ErrorCode() == flare::rpc::EHTTP);
        ASSERT_TRUE(cntl.http_response().status_code() == flare::rpc::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }

    TEST_F(HttpTest, http2_rst_after_header_and_data) {
        flare::rpc::Controller cntl;
        // Prepare request
        flare::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        flare::cord_buf res_out;
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        char rstbuf[9 /*FRAME_HEAD_SIZE*/ + 4];
        flare::rpc::policy::SerializeFrameHead(rstbuf, 4, flare::rpc::policy::H2_FRAME_RST_STREAM, 0, h2_stream_id);
        SaveUint32(rstbuf + 9, flare::rpc::H2_INTERNAL_ERROR);
        res_out.append(rstbuf, sizeof(rstbuf));
        // parse response
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_TRUE(cntl.http_response().status_code() == flare::rpc::HTTP_STATUS_OK);
    }

    TEST_F(HttpTest, http2_window_used_up) {
        flare::rpc::Controller cntl;
        flare::cord_buf request_buf;
        test::EchoRequest req;
        // longer message to trigger using up window size sooner
        req.set_message("FLOW_CONTROL_FLOW_CONTROL");
        cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
        cntl.http_request().set_content_type("application/proto");
        flare::rpc::policy::SerializeHttpRequest(&request_buf, &cntl, &req);

        char settingsbuf[flare::rpc::policy::FRAME_HEAD_SIZE + 36];
        flare::rpc::H2Settings h2_settings;
        const size_t nb = flare::rpc::policy::SerializeH2Settings(h2_settings,
                                                                  settingsbuf + flare::rpc::policy::FRAME_HEAD_SIZE);
        flare::rpc::policy::SerializeFrameHead(settingsbuf, nb, flare::rpc::policy::H2_FRAME_SETTINGS, 0, 0);
        flare::cord_buf buf;
        buf.append(settingsbuf, flare::rpc::policy::FRAME_HEAD_SIZE + nb);
        flare::rpc::policy::ParseH2Message(&buf, _h2_client_sock.get(), false, NULL);

        int nsuc = flare::rpc::H2Settings::DEFAULT_INITIAL_WINDOW_SIZE / cntl.request_attachment().size();
        for (int i = 0; i <= nsuc; i++) {
            flare::rpc::policy::H2UnsentRequest *h2_req = flare::rpc::policy::H2UnsentRequest::New(&cntl);
            cntl._current_call.stream_user_data = h2_req;
            flare::rpc::SocketMessage *socket_message = NULL;
            flare::rpc::policy::PackH2Request(NULL, &socket_message, cntl.call_id().value,
                                              NULL, &cntl, request_buf, NULL);
            flare::cord_buf dummy;
            flare::result_status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
            if (i == nsuc) {
                // the last message should fail according to flow control policy.
                ASSERT_FALSE(st);
                ASSERT_TRUE(st.error_code() == flare::rpc::ELIMIT);
                ASSERT_TRUE(flare::starts_with(st.error_data(), "remote_window_left is not enough"));
            } else {
                ASSERT_TRUE(st);
            }
            h2_req->DestroyStreamUserData(_h2_client_sock, &cntl, 0, false);
        }
    }

    TEST_F(HttpTest, http2_settings) {
        char settingsbuf[flare::rpc::policy::FRAME_HEAD_SIZE + 36];
        flare::rpc::H2Settings h2_settings;
        h2_settings.header_table_size = 8192;
        h2_settings.max_concurrent_streams = 1024;
        h2_settings.stream_window_size = (1u << 29) - 1;
        const size_t nb = flare::rpc::policy::SerializeH2Settings(h2_settings,
                                                                  settingsbuf + flare::rpc::policy::FRAME_HEAD_SIZE);
        flare::rpc::policy::SerializeFrameHead(settingsbuf, nb, flare::rpc::policy::H2_FRAME_SETTINGS, 0, 0);
        flare::cord_buf buf;
        buf.append(settingsbuf, flare::rpc::policy::FRAME_HEAD_SIZE + nb);

        flare::rpc::policy::H2Context *ctx = new flare::rpc::policy::H2Context(_socket.get(), NULL);
        FLARE_CHECK_EQ(ctx->Init(), 0);
        _socket->initialize_parsing_context(&ctx);
        ctx->_conn_state = flare::rpc::policy::H2_CONNECTION_READY;
        // parse settings
        flare::rpc::policy::ParseH2Message(&buf, _socket.get(), false, NULL);

        flare::IOPortal response_buf;
        FLARE_CHECK_EQ(response_buf.append_from_file_descriptor(_pipe_fds[0], 1024),
                       (ssize_t) flare::rpc::policy::FRAME_HEAD_SIZE);
        flare::rpc::policy::H2FrameHead frame_head;
        flare::cord_buf_bytes_iterator it(response_buf);
        ctx->ConsumeFrameHead(it, &frame_head);
        FLARE_CHECK_EQ(frame_head.type, flare::rpc::policy::H2_FRAME_SETTINGS);
        FLARE_CHECK_EQ(frame_head.flags, 0x01 /* H2_FLAGS_ACK */);
        FLARE_CHECK_EQ(frame_head.stream_id, 0);
        ASSERT_TRUE(ctx->_remote_settings.header_table_size == 8192);
        ASSERT_TRUE(ctx->_remote_settings.max_concurrent_streams == 1024);
        ASSERT_TRUE(ctx->_remote_settings.stream_window_size == (1u << 29) - 1);
    }

    TEST_F(HttpTest, http2_invalid_settings) {
        {
            flare::rpc::Server server;
            flare::rpc::ServerOptions options;
            options.h2_settings.stream_window_size = flare::rpc::H2Settings::MAX_WINDOW_SIZE + 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
        {
            flare::rpc::Server server;
            flare::rpc::ServerOptions options;
            options.h2_settings.max_frame_size =
                    flare::rpc::H2Settings::DEFAULT_MAX_FRAME_SIZE - 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
        {
            flare::rpc::Server server;
            flare::rpc::ServerOptions options;
            options.h2_settings.max_frame_size =
                    flare::rpc::H2Settings::MAX_OF_MAX_FRAME_SIZE + 1;
            ASSERT_EQ(-1, server.Start("127.0.0.1:8924", &options));
        }
    }

    TEST_F(HttpTest, http2_not_closing_socket_when_rpc_timeout) {
        const int port = 8923;
        flare::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, NULL));
        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = "h2";
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));

        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        {
            // make a successful call to create the connection first
            flare::rpc::Controller cntl;
            cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(NULL, &cntl, &req, &res, NULL);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
        }

        flare::rpc::SocketUniquePtr main_ptr;
        EXPECT_EQ(flare::rpc::Socket::Address(channel._server_id, &main_ptr), 0);
        flare::rpc::SocketId agent_id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);

        for (int i = 0; i < 4; i++) {
            flare::rpc::Controller cntl;
            cntl.set_timeout_ms(50);
            cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo?sleep_ms=300";
            channel.CallMethod(NULL, &cntl, &req, &res, NULL);
            ASSERT_TRUE(cntl.Failed());

            flare::rpc::SocketUniquePtr ptr;
            flare::rpc::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
            EXPECT_EQ(id, agent_id);
        }

        {
            // make a successful call again to make sure agent_socket not changing
            flare::rpc::Controller cntl;
            cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
            cntl.http_request().uri() = "/EchoService/Echo";
            channel.CallMethod(NULL, &cntl, &req, &res, NULL);
            ASSERT_FALSE(cntl.Failed());
            ASSERT_EQ(EXP_RESPONSE, res.message());
            flare::rpc::SocketUniquePtr ptr;
            flare::rpc::SocketId id = main_ptr->_agent_socket_id.load(std::memory_order_relaxed);
            EXPECT_EQ(id, agent_id);
        }
    }

    TEST_F(HttpTest, http2_header_after_data) {
        flare::rpc::Controller cntl;

        // Prepare request
        flare::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);

        // Prepare response to res_out
        flare::cord_buf res_out;
        {
            flare::cord_buf data_buf;
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            {
                flare::cord_buf_as_zero_copy_output_stream wrapper(&data_buf);
                EXPECT_TRUE(res.SerializeToZeroCopyStream(&wrapper));
            }
            flare::rpc::policy::H2Context *ctx =
                    static_cast<flare::rpc::policy::H2Context *>(_h2_client_sock->parsing_context());
            flare::rpc::HPacker &hpacker = ctx->hpacker();
            flare::cord_buf_appender header1_appender;
            flare::rpc::HPackOptions options;
            options.encode_name = false;    /* disable huffman encoding */
            options.encode_value = false;
            {
                flare::rpc::HPacker::Header header(":status", "200");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                flare::rpc::HPacker::Header header("content-length",
                                                   flare::string_printf("%llu", (unsigned long long) data_buf.size()));
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                flare::rpc::HPacker::Header header(":status", "200");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                flare::rpc::HPacker::Header header("content-type", "application/proto");
                hpacker.Encode(&header1_appender, header, options);
            }
            {
                flare::rpc::HPacker::Header header("user-defined1", "a");
                hpacker.Encode(&header1_appender, header, options);
            }
            flare::cord_buf header1;
            header1_appender.move_to(header1);

            char headbuf[flare::rpc::policy::FRAME_HEAD_SIZE];
            flare::rpc::policy::SerializeFrameHead(headbuf, header1.size(),
                                                   flare::rpc::policy::H2_FRAME_HEADERS, 0, h2_stream_id);
            // append header1
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(flare::cord_buf::Movable(header1));

            flare::rpc::policy::SerializeFrameHead(headbuf, data_buf.size(),
                                                   flare::rpc::policy::H2_FRAME_DATA, 0, h2_stream_id);
            // append data
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(flare::cord_buf::Movable(data_buf));

            flare::cord_buf_appender header2_appender;
            {
                flare::rpc::HPacker::Header header("user-defined1", "overwrite-a");
                hpacker.Encode(&header2_appender, header, options);
            }
            {
                flare::rpc::HPacker::Header header("user-defined2", "b");
                hpacker.Encode(&header2_appender, header, options);
            }
            flare::cord_buf header2;
            header2_appender.move_to(header2);

            flare::rpc::policy::SerializeFrameHead(headbuf, header2.size(),
                                                   flare::rpc::policy::H2_FRAME_HEADERS,
                                                   0x05/* end header and stream */,
                                                   h2_stream_id);
            // append header2
            res_out.append(headbuf, sizeof(headbuf));
            res_out.append(flare::cord_buf::Movable(header2));
        }
        // parse response
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_FALSE(cntl.Failed());

        flare::rpc::HttpHeader &res_header = cntl.http_response();
        ASSERT_EQ(res_header.content_type(), "application/proto");
        // Check overlapped header is overwritten by the latter.
        const std::string *user_defined1 = res_header.GetHeader("user-defined1");
        ASSERT_EQ(*user_defined1, "overwrite-a");
        const std::string *user_defined2 = res_header.GetHeader("user-defined2");
        ASSERT_EQ(*user_defined2, "b");
    }

    TEST_F(HttpTest, http2_goaway_sanity) {
        flare::rpc::Controller cntl;
        // Prepare request
        flare::cord_buf req_out;
        int h2_stream_id = 0;
        MakeH2EchoRequestBuf(&req_out, &cntl, &h2_stream_id);
        // Prepare response
        flare::cord_buf res_out;
        MakeH2EchoResponseBuf(&res_out, h2_stream_id);
        // append goaway
        char goawaybuf[9 /*FRAME_HEAD_SIZE*/ + 8];
        flare::rpc::policy::SerializeFrameHead(goawaybuf, 8, flare::rpc::policy::H2_FRAME_GOAWAY, 0, 0);
        SaveUint32(goawaybuf + 9, 0x7fffd8ef /*last stream id*/);
        SaveUint32(goawaybuf + 13, flare::rpc::H2_NO_ERROR);
        res_out.append(goawaybuf, sizeof(goawaybuf));
        // parse response
        flare::rpc::ParseResult res_pr =
                flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_TRUE(res_pr.is_ok());
        // process response
        ProcessMessage(flare::rpc::policy::ProcessHttpResponse, res_pr.message(), false);
        ASSERT_TRUE(!cntl.Failed());

        // parse GOAWAY
        res_pr = flare::rpc::policy::ParseH2Message(&res_out, _h2_client_sock.get(), false, NULL);
        ASSERT_EQ(res_pr.error(), flare::rpc::PARSE_ERROR_NOT_ENOUGH_DATA);

        // Since GOAWAY has been received, the next request should fail
        flare::rpc::policy::H2UnsentRequest *h2_req = flare::rpc::policy::H2UnsentRequest::New(&cntl);
        cntl._current_call.stream_user_data = h2_req;
        flare::rpc::SocketMessage *socket_message = NULL;
        flare::rpc::policy::PackH2Request(NULL, &socket_message, cntl.call_id().value,
                                          NULL, &cntl, flare::cord_buf(), NULL);
        flare::cord_buf dummy;
        flare::result_status st = socket_message->AppendAndDestroySelf(&dummy, _h2_client_sock.get());
        ASSERT_EQ(st.error_code(), flare::rpc::ELOGOFF);
        ASSERT_TRUE(flare::ends_with(st.error_data(), "the connection just issued GOAWAY"));
    }

    class AfterRecevingGoAway : public ::google::protobuf::Closure {
    public:
        void Run() {
            ASSERT_EQ(flare::rpc::EHTTP, cntl.ErrorCode());
            delete this;
        }

        flare::rpc::Controller cntl;
    };

    TEST_F(HttpTest, http2_handle_goaway_streams) {
        const flare::base::end_point ep(flare::base::IP_ANY, 5961);
        flare::base::fd_guard listenfd(flare::base::tcp_listen(ep));
        ASSERT_GT(listenfd, 0);

        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = flare::rpc::PROTOCOL_H2;
        ASSERT_EQ(0, channel.Init(ep, &options));

        int req_size = 10;
        std::vector<flare::rpc::CallId> ids(req_size);
        for (int i = 0; i < req_size; i++) {
            AfterRecevingGoAway *done = new AfterRecevingGoAway;
            flare::rpc::Controller &cntl = done->cntl;
            ids.push_back(cntl.call_id());
            cntl.set_timeout_ms(-1);
            cntl.http_request().uri() = "/it-doesnt-matter";
            channel.CallMethod(NULL, &cntl, NULL, NULL, done);
        }

        int servfd = accept(listenfd, NULL, NULL);
        ASSERT_GT(servfd, 0);
        // Sleep for a while to make sure that server has received all data.
        flare::fiber_sleep_for(2000);
        char goawaybuf[flare::rpc::policy::FRAME_HEAD_SIZE + 8];
        SerializeFrameHead(goawaybuf, 8, flare::rpc::policy::H2_FRAME_GOAWAY, 0, 0);
        SaveUint32(goawaybuf + flare::rpc::policy::FRAME_HEAD_SIZE, 0);
        SaveUint32(goawaybuf + flare::rpc::policy::FRAME_HEAD_SIZE + 4, 0);
        ASSERT_EQ((ssize_t) flare::rpc::policy::FRAME_HEAD_SIZE + 8,
                  ::write(servfd, goawaybuf, flare::rpc::policy::FRAME_HEAD_SIZE + 8));

        // After receving GOAWAY, the callbacks in client should be run correctly.
        for (int i = 0; i < req_size; i++) {
            flare::rpc::Join(ids[i]);
        }
    }

    TEST_F(HttpTest, spring_protobuf_content_type) {
        const int port = 8923;
        flare::rpc::Server server;
        EXPECT_EQ(0, server.AddService(&_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        EXPECT_EQ(0, server.Start(port, nullptr));

        flare::rpc::Channel channel;
        flare::rpc::ChannelOptions options;
        options.protocol = "http";
        ASSERT_EQ(0, channel.Init(flare::base::end_point(flare::base::my_ip(), port), &options));

        flare::rpc::Controller cntl;
        test::EchoRequest req;
        test::EchoResponse res;
        req.set_message(EXP_REQUEST);
        cntl.http_request().set_method(flare::rpc::HTTP_METHOD_POST);
        cntl.http_request().uri() = "/EchoService/Echo";
        cntl.http_request().set_content_type("application/x-protobuf");
        cntl.request_attachment().append(req.SerializeAsString());
        channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
        ASSERT_TRUE(res.ParseFromString(cntl.response_attachment().to_string()));
        ASSERT_EQ(EXP_RESPONSE, res.message());

        flare::rpc::Controller cntl2;
        test::EchoService_Stub stub(&channel);
        req.set_message(EXP_REQUEST);
        res.Clear();
        cntl2.http_request().set_content_type("application/x-protobuf");
        stub.Echo(&cntl2, &req, &res, nullptr);
        ASSERT_FALSE(cntl.Failed());
        ASSERT_EQ(EXP_RESPONSE, res.message());
        ASSERT_EQ("application/x-protobuf", cntl.http_response().content_type());
    }

} //namespace
