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
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <google/protobuf/descriptor.h>
#include "flare/times/time.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/acceptor.h"
#include "flare/rpc/server.h"
#include "flare/rpc/policy/nova_pbrpc_protocol.h"
#include "flare/rpc/policy/most_common_message.h"
#include "flare/rpc/controller.h"
#include "echo.pb.h"

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";

static const std::string MOCK_CREDENTIAL = "mock credential";
static const std::string MOCK_USER = "mock user";

class MyAuthenticator : public flare::rpc::Authenticator {
public:
    MyAuthenticator() {}

    int GenerateCredential(std::string* auth_str) const {
        *auth_str = MOCK_CREDENTIAL;
        return 0;
    }

    int VerifyCredential(const std::string& auth_str,
                         const flare::base::end_point&,
                         flare::rpc::AuthContext* ctx) const {
        EXPECT_EQ(MOCK_CREDENTIAL, auth_str);
        ctx->set_user(MOCK_USER);
        return 0;
    }
};

class MyEchoService : public ::test::EchoService {
    void Echo(::google::protobuf::RpcController* cntl_base,
              const ::test::EchoRequest* req,
              ::test::EchoResponse* res,
              ::google::protobuf::Closure* done) {
        flare::rpc::Controller* cntl =
            static_cast<flare::rpc::Controller*>(cntl_base);
        flare::rpc::ClosureGuard done_guard(done);

        if (req->close_fd()) {
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        res->set_message(EXP_RESPONSE);
    }
};
    
class NovaTest : public ::testing::Test{
protected:
    NovaTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = flare::rpc::Server::RUNNING;
        _server._options.nshead_service = new flare::rpc::policy::NovaServiceAdaptor;
        // Nova doesn't support authentication
        // _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        flare::rpc::SocketId id;
        flare::rpc::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, flare::rpc::Socket::Create(options, &id));
        EXPECT_EQ(0, flare::rpc::Socket::Address(id, &_socket));
    };

    virtual ~NovaTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void VerifyMessage(flare::rpc::InputMessageBase* msg) {
        if (msg->_socket == NULL) {
            _socket->ReAddress(&msg->_socket);
        }
        msg->_arg = &_server;
        EXPECT_TRUE(flare::rpc::policy::VerifyNsheadRequest(msg));
    }

    void ProcessMessage(void (*process)(flare::rpc::InputMessageBase*),
                        flare::rpc::InputMessageBase* msg, bool set_eof) {
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

    flare::rpc::policy::MostCommonMessage* MakeRequestMessage(
        const flare::rpc::nshead_t& head) {
        flare::rpc::policy::MostCommonMessage* msg =
                flare::rpc::policy::MostCommonMessage::Get();
        msg->meta.append(&head, sizeof(head));

        test::EchoRequest req;
        req.set_message(EXP_REQUEST);
        flare::cord_buf_as_zero_copy_output_stream req_stream(&msg->payload);
        EXPECT_TRUE(req.SerializeToZeroCopyStream(&req_stream));
        return msg;
    }

    flare::rpc::policy::MostCommonMessage* MakeResponseMessage() {
        flare::rpc::policy::MostCommonMessage* msg =
                flare::rpc::policy::MostCommonMessage::Get();
        flare::rpc::nshead_t head;
        memset(&head, 0, sizeof(head));
        msg->meta.append(&head, sizeof(head));
        
        test::EchoResponse res;
        res.set_message(EXP_RESPONSE);
        flare::cord_buf_as_zero_copy_output_stream res_stream(&msg->payload);
        EXPECT_TRUE(res.SerializeToZeroCopyStream(&res_stream));
        return msg;
    }

    void CheckEmptyResponse() {
        int bytes_in_pipe = 0;
        ioctl(_pipe_fds[0], FIONREAD, &bytes_in_pipe);
        EXPECT_EQ(0, bytes_in_pipe);
    }

    int _pipe_fds[2];
    flare::rpc::SocketUniquePtr _socket;
    flare::rpc::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(NovaTest, process_request_failed_socket) {
    flare::rpc::nshead_t head;
    memset(&head, 0, sizeof(head));
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    _socket->SetFailed();
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_request_logoff) {
    flare::rpc::nshead_t head;
    head.reserved = 0;
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    _server._status = flare::rpc::Server::READY;
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    ASSERT_TRUE(_socket->Failed());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_request_wrong_method) {
    flare::rpc::nshead_t head;
    head.reserved = 10;
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(head);
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    ASSERT_TRUE(_socket->Failed());
    CheckEmptyResponse();
}

TEST_F(NovaTest, process_response_after_eof) {
    test::EchoResponse res;
    flare::rpc::Controller cntl;
    cntl._response = &res;
    flare::rpc::policy::MostCommonMessage* msg = MakeResponseMessage();
    _socket->set_correlation_id(cntl.call_id().value);
    ProcessMessage(flare::rpc::policy::ProcessNovaResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(NovaTest, complete_flow) {
    flare::cord_buf request_buf;
    flare::cord_buf total_buf;
    flare::rpc::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;
    cntl._connection_type = flare::rpc::CONNECTION_TYPE_SHORT;
    ASSERT_EQ(0, flare::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    // Send request
    req.set_message(EXP_REQUEST);
    flare::rpc::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    flare::rpc::policy::PackNovaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Verify and handle request
    flare::rpc::ParseResult req_pr =
            flare::rpc::policy::ParseNsheadMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(flare::rpc::PARSE_OK, req_pr.error());
    flare::rpc::InputMessageBase* req_msg = req_pr.message();
    VerifyMessage(req_msg);
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, req_msg, false);

    // Read response from pipe
    flare::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);
    flare::rpc::ParseResult res_pr =
            flare::rpc::policy::ParseNsheadMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(flare::rpc::PARSE_OK, res_pr.error());
    flare::rpc::InputMessageBase* res_msg = res_pr.message();
    ProcessMessage(flare::rpc::policy::ProcessNovaResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(NovaTest, close_in_callback) {
    flare::cord_buf request_buf;
    flare::cord_buf total_buf;
    flare::rpc::Controller cntl;
    test::EchoRequest req;
    cntl._connection_type = flare::rpc::CONNECTION_TYPE_SHORT;
    ASSERT_EQ(0, flare::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    flare::rpc::SerializeRequestDefault(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    flare::rpc::policy::PackNovaRequest(
        &total_buf, NULL, cntl.call_id().value,
        test::EchoService::descriptor()->method(0), &cntl, request_buf, &_auth);
    ASSERT_FALSE(cntl.Failed());

    // Handle request
    flare::rpc::ParseResult req_pr =
            flare::rpc::policy::ParseNsheadMessage(&total_buf, NULL, false, NULL);
    ASSERT_EQ(flare::rpc::PARSE_OK, req_pr.error());
    flare::rpc::InputMessageBase* req_msg = req_pr.message();
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, req_msg, false);

    // Socket should be closed
    ASSERT_TRUE(_socket->Failed());
}
} //namespace
