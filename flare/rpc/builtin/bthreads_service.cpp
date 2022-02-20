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


#include <ostream>
#include "flare/rpc/closure_guard.h"        // ClosureGuard
#include "flare/rpc/controller.h"           // Controller
#include "flare/rpc/builtin/common.h"
#include "flare/rpc/builtin/bthreads_service.h"

namespace flare::fiber_internal {
void print_task(std::ostream& os, fiber_id_t tid);
}


namespace flare::rpc {

void BthreadsService::default_method(::google::protobuf::RpcController* cntl_base,
                                     const ::flare::rpc::BthreadsRequest*,
                                     ::flare::rpc::BthreadsResponse*,
                                     ::google::protobuf::Closure* done) {
    ClosureGuard done_guard(done);
    Controller *cntl = static_cast<Controller*>(cntl_base);
    cntl->http_response().set_content_type("text/plain");
    flare::io::cord_buf_builder os;
    const std::string& constraint = cntl->http_request().unresolved_path();
    
    if (constraint.empty()) {
        os << "Use /bthreads/<bthread_id>";
    } else {
        char* endptr = NULL;
        fiber_id_t tid = strtoull(constraint.c_str(), &endptr, 10);
        if (*endptr == '\0' || *endptr == '/') {
            ::flare::fiber_internal::print_task(os, tid);
        } else {
            cntl->SetFailed(ENOMETHOD, "path=%s is not a fiber id",
                            constraint.c_str());
        }
    }
    os.move_to(cntl->response_attachment());
}

} // namespace flare::rpc
