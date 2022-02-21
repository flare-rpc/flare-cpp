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


#include "flare/rpc/controller.h"           // Controller
#include "flare/rpc/server.h"               // Server
#include "flare/rpc/closure_guard.h"        // ClosureGuard
#include "flare/rpc/builtin/version_service.h"


namespace flare::rpc {

void VersionService::default_method(::google::protobuf::RpcController* controller,
                                    const ::flare::rpc::VersionRequest*,
                                    ::flare::rpc::VersionResponse*,
                                    ::google::protobuf::Closure* done) {
    ClosureGuard done_guard(done);
    Controller *cntl = (Controller *)controller;
    cntl->http_response().set_content_type("text/plain");
    if (_server->version().empty()) {
        cntl->response_attachment().append("unknown");
    } else {
        cntl->response_attachment().append(_server->version());
    }
}

} // namespace flare::rpc
