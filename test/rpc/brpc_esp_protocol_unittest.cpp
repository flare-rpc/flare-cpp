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
#include "flare/base/time.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/policy/most_common_message.h"
#include "flare/rpc/controller.h"

#include "flare/rpc/esp_message.h"
#include "flare/rpc/policy/esp_protocol.h"
#include "flare/rpc/policy/esp_authenticator.h"

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

namespace {

static const std::string EXP_REQUEST = "hello";
static const std::string EXP_RESPONSE = "world";

static const int STUB = 2;
static const int MSG_ID = 123456;
static const int MSG = 0;
static const int WRONG_MSG = 1;

class EspTest : public ::testing::Test{
protected:
    EspTest() {
        EXPECT_EQ(0, pipe(_pipe_fds));
        flare::rpc::SocketId id;
        flare::rpc::SocketOptions options;
        options.fd = _pipe_fds[1];
        EXPECT_EQ(0, flare::rpc::Socket::Create(options, &id));
        EXPECT_EQ(0, flare::rpc::Socket::Address(id, &_socket));
    };

    virtual ~EspTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    void WriteResponse(flare::rpc::Controller& cntl, int msg) {
        flare::rpc::EspMessage req;
    
        req.head.to.stub = STUB;
        req.head.msg = msg;
        req.head.msg_id = MSG_ID;
        req.body.append(EXP_RESPONSE);
    
        flare::io::cord_buf req_buf;
        flare::rpc::policy::SerializeEspRequest(&req_buf, &cntl, &req);
    
        flare::io::cord_buf packet_buf;
        flare::rpc::policy::PackEspRequest(&packet_buf, NULL, cntl.call_id().value, NULL, &cntl, req_buf, NULL);
    
        packet_buf.cut_into_file_descriptor(_pipe_fds[1], packet_buf.size());
    }

    int _pipe_fds[2];
    flare::rpc::SocketUniquePtr _socket;
};

TEST_F(EspTest, complete_flow) {
    flare::rpc::EspMessage req;
    flare::rpc::EspMessage res;

    req.head.to.stub = STUB;
    req.head.msg = MSG;
    req.head.msg_id = MSG_ID;
    req.body.append(EXP_REQUEST);

    flare::io::cord_buf req_buf;
    flare::rpc::Controller cntl;
    cntl._response = &res;
    ASSERT_EQ(0, flare::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    flare::rpc::policy::SerializeEspRequest(&req_buf, &cntl, &req);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(sizeof(req.head) + req.body.size(), req_buf.size());

    const flare::rpc::Authenticator* auth = flare::rpc::policy::global_esp_authenticator();
    flare::io::cord_buf packet_buf;
    flare::rpc::policy::PackEspRequest(&packet_buf, NULL, cntl.call_id().value, NULL, &cntl, req_buf, auth);

    std::string auth_str;
    auth->GenerateCredential(&auth_str);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(req_buf.size() + auth_str.size(), packet_buf.size());

    WriteResponse(cntl, MSG);

    flare::io::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);

    flare::rpc::ParseResult res_pr =
            flare::rpc::policy::ParseEspMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(flare::rpc::PARSE_OK, res_pr.error());

    flare::rpc::InputMessageBase* res_msg = res_pr.message();
    _socket->ReAddress(&res_msg->_socket);

    flare::rpc::policy::ProcessEspResponse(res_msg);

    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(EXP_RESPONSE, res.body.to_string());
}

TEST_F(EspTest, wrong_response_head) {
    flare::rpc::EspMessage res;
    flare::rpc::Controller cntl;
    cntl._response = &res;
    ASSERT_EQ(0, flare::rpc::Socket::Address(_socket->id(), &cntl._current_call.sending_sock));

    WriteResponse(cntl, WRONG_MSG);

    flare::io::IOPortal response_buf;
    response_buf.append_from_file_descriptor(_pipe_fds[0], 1024);

    flare::rpc::ParseResult res_pr =
            flare::rpc::policy::ParseEspMessage(&response_buf, NULL, false, NULL);
    ASSERT_EQ(flare::rpc::PARSE_OK, res_pr.error());

    flare::rpc::InputMessageBase* res_msg = res_pr.message();
    _socket->ReAddress(&res_msg->_socket);

    flare::rpc::policy::ProcessEspResponse(res_msg);

    ASSERT_TRUE(cntl.Failed());
}
} //namespace
