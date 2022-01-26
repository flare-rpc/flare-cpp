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


#ifndef BRPC_POLICY_SOFA_PBRPC_PROTOCOL_H
#define BRPC_POLICY_SOFA_PBRPC_PROTOCOL_H

#include "flare/rpc/policy/sofa_pbrpc_meta.pb.h"
#include "flare/rpc/protocol.h"


namespace flare::rpc {
namespace policy {

// Parse binary format of sofa-pbrpc.
ParseResult ParseSofaMessage(flare::io::cord_buf* source, Socket *socket, bool read_eof, const void *arg);

// Actions to a (client) request in sofa-pbrpc format.
void ProcessSofaRequest(InputMessageBase* msg);

// Actions to a (server) response in sofa-pbrpc format.
void ProcessSofaResponse(InputMessageBase* msg);

// Verify authentication information in sofa-pbrpc format
bool VerifySofaRequest(const InputMessageBase* msg);

// Pack `request' to `method' into `buf'.
void PackSofaRequest(flare::io::cord_buf* buf,
                     SocketMessage**,
                     uint64_t correlation_id,
                     const google::protobuf::MethodDescriptor* method,
                     Controller* controller,
                     const flare::io::cord_buf& request,
                     const Authenticator* auth);

}  // namespace policy
} // namespace flare::rpc


#endif  // BRPC_POLICY_SOFA_PBRPC_PROTOCOL_H
