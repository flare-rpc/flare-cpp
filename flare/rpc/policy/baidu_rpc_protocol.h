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


#ifndef FLARE_RPC_POLICY_BRPC_PROTOCOL_H_
#define FLARE_RPC_POLICY_BRPC_PROTOCOL_H_

#include "flare/rpc/protocol.h"

namespace flare::rpc {
    namespace policy {

        // Parse binary format of baidu_std
        ParseResult ParseRpcMessage(flare::cord_buf *source, Socket *socket, bool read_eof,
                                    const void *arg);

        // Actions to a (client) request in baidu_std format
        void ProcessRpcRequest(InputMessageBase *msg);

        // Actions to a (server) response in baidu_std format.
        void ProcessRpcResponse(InputMessageBase *msg);

        // Verify authentication information in baidu_std format
        bool VerifyRpcRequest(const InputMessageBase *msg);

        // Pack `request' to `method' into `buf'.
        void PackRpcRequest(flare::cord_buf *buf,
                            SocketMessage **,
                            uint64_t correlation_id,
                            const google::protobuf::MethodDescriptor *method,
                            Controller *controller,
                            const flare::cord_buf &request,
                            const Authenticator *auth);

    }  // namespace policy
} // namespace flare::rpc

#endif  // FLARE_RPC_POLICY_BRPC_PROTOCOL_H_
