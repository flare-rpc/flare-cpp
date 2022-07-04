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
#include "flare/rpc/socket.h"
#include "flare/rpc/acceptor.h"
#include "flare/rpc/server.h"
#include "flare/rpc/policy/public_pbrpc_meta.pb.h"
#include "flare/rpc/policy/public_pbrpc_protocol.h"
#include "flare/rpc/policy/most_common_message.h"
#include "flare/rpc/controller.h"
#include "echo.pb.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
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
        flare::rpc::Controller* cntl = static_cast<flare::rpc::Controller*>(cntl_base);
        flare::rpc::ClosureGuard done_guard(done);

        if (req->close_fd()) {
            cntl->CloseConnection("Close connection according to request");
            return;
        }
        EXPECT_EQ(EXP_REQUEST, req->message());
        res->set_message(EXP_RESPONSE);
    }
};
    
class PublicPbrpcTest : public ::testing::Test{
protected:
    PublicPbrpcTest() {
        EXPECT_EQ(0, _server.AddService(
            &_svc, flare::rpc::SERVER_DOESNT_OWN_SERVICE));
        // Hack: Regard `_server' as running 
        _server._status = flare::rpc::Server::RUNNING;
        _server._options.nshead_service =
            new flare::rpc::policy::PublicPbrpcServiceAdaptor;
        // public_pbrpc doesn't support authentication
        // _server._options.auth = &_auth;
        
        EXPECT_EQ(0, pipe(_pipe_fds));

        flare::rpc::SocketId id;
        flare::rpc::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, flare::rpc::Socket::Create(options, &id));
        EXPECT_EQ(0, flare::rpc::Socket::Address(id, &_socket));
    };

    virtual ~PublicPbrpcTest() {};
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
        flare::rpc::policy::PublicPbrpcRequest* meta) {
        flare::rpc::policy::MostCommonMessage* msg =
                flare::rpc::policy::MostCommonMessage::Get();
        flare::rpc::nshead_t head;
        msg->meta.append(&head, sizeof(head));

        if (meta->requestbody_size() > 0) {
            test::EchoRequest req;
            req.set_message(EXP_REQUEST);
            EXPECT_TRUE(req.SerializeToString(
                meta->mutable_requestbody(0)->mutable_serialized_request()));
        }
        flare::cord_buf_as_zero_copy_output_stream meta_stream(&msg->payload);
        EXPECT_TRUE(meta->SerializeToZeroCopyStream(&meta_stream));
        return msg;
    }

    flare::rpc::policy::MostCommonMessage* MakeResponseMessage(
        flare::rpc::policy::PublicPbrpcResponse* meta) {
        flare::rpc::policy::MostCommonMessage* msg =
                flare::rpc::policy::MostCommonMessage::Get();
        flare::rpc::nshead_t head;
        msg->meta.append(&head, sizeof(head));

        if (meta->responsebody_size() > 0) {
            test::EchoResponse res;
            res.set_message(EXP_RESPONSE);
            EXPECT_TRUE(res.SerializeToString(
                meta->mutable_responsebody(0)->mutable_serialized_response()));
        }
        flare::cord_buf_as_zero_copy_output_stream meta_stream(&msg->payload);
        EXPECT_TRUE(meta->SerializeToZeroCopyStream(&meta_stream));
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
        EXPECT_EQ((ssize_t)bytes_in_pipe,
                  buf.append_from_file_descriptor(_pipe_fds[0], 1024));
        flare::rpc::ParseResult pr = flare::rpc::policy::ParseNsheadMessage(&buf, NULL, false, NULL);
        EXPECT_EQ(flare::rpc::PARSE_OK, pr.error());
        flare::rpc::policy::MostCommonMessage* msg =
            static_cast<flare::rpc::policy::MostCommonMessage*>(pr.message());

        flare::rpc::policy::PublicPbrpcResponse meta;
        flare::cord_buf_as_zero_copy_input_stream meta_stream(msg->payload);
        EXPECT_TRUE(meta.ParseFromZeroCopyStream(&meta_stream));
        EXPECT_EQ(expect_code, meta.responsehead().code());
    }

    int _pipe_fds[2];
    flare::rpc::SocketUniquePtr _socket;
    flare::rpc::Server _server;

    MyEchoService _svc;
    MyAuthenticator _auth;
};

TEST_F(PublicPbrpcTest, process_request_failed_socket) {
    flare::rpc::policy::PublicPbrpcRequest meta;
    flare::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(0);
    body->set_id(0);
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    _socket->SetFailed();
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(0ll, _server._nerror_var.get_value());
    CheckResponseCode(true, 0);
}

TEST_F(PublicPbrpcTest, process_request_logoff) {
    flare::rpc::policy::PublicPbrpcRequest meta;
    flare::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(0);
    body->set_id(0);
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    _server._status = flare::rpc::Server::READY;
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    CheckResponseCode(false, flare::rpc::ELOGOFF);
}

TEST_F(PublicPbrpcTest, process_request_wrong_method) {
    flare::rpc::policy::PublicPbrpcRequest meta;
    flare::rpc::policy::RequestBody* body = meta.add_requestbody();
    body->set_service("EchoService");
    body->set_method_id(10);
    body->set_id(0);
    flare::rpc::policy::MostCommonMessage* msg = MakeRequestMessage(&meta);
    ProcessMessage(flare::rpc::policy::ProcessNsheadRequest, msg, false);
    ASSERT_EQ(1ll, _server._nerror_var.get_value());
    ASSERT_FALSE(_socket->Failed());
}

TEST_F(PublicPbrpcTest, process_response_after_eof) {
    flare::rpc::policy::PublicPbrpcResponse meta;
    test::EchoResponse res;
    flare::rpc::Controller cntl;
    flare::rpc::policy::ResponseBody* body = meta.add_responsebody();
    body->set_id(cntl.call_id().value);
    meta.mutable_responsehead()->set_code(0);
    cntl._response = &res;
    flare::rpc::policy::MostCommonMessage* msg = MakeResponseMessage(&meta);
    ProcessMessage(flare::rpc::policy::ProcessPublicPbrpcResponse, msg, true);
    ASSERT_EQ(EXP_RESPONSE, res.message());
    ASSERT_TRUE(_socket->Failed());
}

TEST_F(PublicPbrpcTest, process_response_error_code) {
    const int ERROR_CODE = 12345;
    flare::rpc::policy::PublicPbrpcResponse meta;
    flare::rpc::Controller cntl;
    flare::rpc::policy::ResponseBody* body = meta.add_responsebody();
    body->set_id(cntl.call_id().value);
    meta.mutable_responsehead()->set_code(ERROR_CODE);
    flare::rpc::policy::MostCommonMessage* msg = MakeResponseMessage(&meta);
    ProcessMessage(flare::rpc::policy::ProcessPublicPbrpcResponse, msg, false);
    ASSERT_EQ(ERROR_CODE, cntl.ErrorCode());
}

TEST_F(PublicPbrpcTest, complete_flow) {
    flare::cord_buf request_buf;
    flare::cord_buf total_buf;
    flare::rpc::Controller cntl;
    test::EchoRequest req;
    test::EchoResponse res;
    cntl._response = &res;

    // Send request
    req.set_message(EXP_REQUEST);
    cntl.set_request_compress_type(flare::rpc::COMPRESS_TYPE_SNAPPY);
    flare::rpc::policy::SerializePublicPbrpcRequest(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    flare::rpc::policy::PackPublicPbrpcRequest(
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
    ProcessMessage(flare::rpc::policy::ProcessPublicPbrpcResponse, res_msg, false);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.message());
}

TEST_F(PublicPbrpcTest, close_in_callback) {
    flare::cord_buf request_buf;
    flare::cord_buf total_buf;
    flare::rpc::Controller cntl;
    test::EchoRequest req;

    // Send request
    req.set_message(EXP_REQUEST);
    req.set_close_fd(true);
    flare::rpc::policy::SerializePublicPbrpcRequest(&request_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    flare::rpc::policy::PackPublicPbrpcRequest(
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
