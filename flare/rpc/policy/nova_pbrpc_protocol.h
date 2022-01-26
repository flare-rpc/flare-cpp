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


#ifndef BRPC_POLICY_NOVA_PBRPC_PROTOCOL_H
#define BRPC_POLICY_NOVA_PBRPC_PROTOCOL_H

#include "flare/rpc/nshead_pb_service_adaptor.h"
#include "flare/rpc/policy/nshead_protocol.h"


namespace flare::rpc {
namespace policy {

// Actions to a (server) response in nova_pbrpc format.
void ProcessNovaResponse(InputMessageBase* msg);

void SerializeNovaRequest(flare::io::IOBuf* buf, Controller* cntl,
                                 const google::protobuf::Message* request);

// Pack `request' to `method' into `buf'.
void PackNovaRequest(flare::io::IOBuf* buf,
                     SocketMessage** user_message_out,
                     uint64_t correlation_id,
                     const google::protobuf::MethodDescriptor* method,
                     Controller* controller,
                     const flare::io::IOBuf& request,
                     const Authenticator* auth);

class NovaServiceAdaptor : public NsheadPbServiceAdaptor {
public:
    void ParseNsheadMeta(const Server& svr,
                        const NsheadMessage& request,
                        Controller*,
                        NsheadMeta* out_meta) const;

    void ParseRequestFromIOBuf(
        const NsheadMeta& meta, const NsheadMessage& ns_req,
        Controller* controller, google::protobuf::Message* pb_req) const;

    void SerializeResponseToIOBuf(
        const NsheadMeta& meta,
        Controller* controller,
        const google::protobuf::Message* pb_res,
        NsheadMessage* ns_res) const;
};

}  // namespace policy
} // namespace flare::rpc


#endif // BRPC_POLICY_NOVA_PBRPC_PROTOCOL_H
