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


#ifndef BRPC_POLICY_HULU_PBRPC_PROTOCOL_H
#define BRPC_POLICY_HULU_PBRPC_PROTOCOL_H

#include "flare/rpc/policy/hulu_pbrpc_meta.pb.h"
#include "flare/rpc/protocol.h"

namespace flare::rpc {
namespace policy {

// Parse binary format of hulu-pbrpc.
ParseResult ParseHuluMessage(flare::io::IOBuf* source, Socket *socket, bool read_eof, const void *arg);

// Actions to a (client) request in hulu-pbrpc format.
void ProcessHuluRequest(InputMessageBase* msg);

// Actions to a (server) response in hulu-pbrpc format.
void ProcessHuluResponse(InputMessageBase* msg);

// Verify authentication information in hulu-pbrpc format
bool VerifyHuluRequest(const InputMessageBase* msg);

// Pack `request' to `method' into `buf'.
void PackHuluRequest(flare::io::IOBuf* buf,
                     SocketMessage**,
                     uint64_t correlation_id,
                     const google::protobuf::MethodDescriptor* method,
                     Controller* controller,
                     const flare::io::IOBuf& request,
                     const Authenticator* auth);

}  // namespace policy
} // namespace flare::rpc


#endif  // BRPC_POLICY_HULU_PBRPC_PROTOCOL_H
