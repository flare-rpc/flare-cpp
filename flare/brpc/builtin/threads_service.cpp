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


#include "flare/base/time.h"
#include "flare/base/logging.h"
#include "flare/base/popen.h"
#include "flare/brpc/controller.h"           // Controller
#include "flare/brpc/closure_guard.h"        // ClosureGuard
#include "flare/brpc/builtin/threads_service.h"
#include "flare/brpc/builtin/common.h"
#include "flare/base/strings.h"

namespace brpc {

void ThreadsService::default_method(::google::protobuf::RpcController* cntl_base,
                                    const ::brpc::ThreadsRequest*,
                                    ::brpc::ThreadsResponse*,
                                    ::google::protobuf::Closure* done) {
    ClosureGuard done_guard(done);
    Controller *cntl = static_cast<Controller*>(cntl_base);
    cntl->http_response().set_content_type("text/plain");
    flare::io::IOBuf& resp = cntl->response_attachment();

    std::string cmd = flare::base::string_printf("pstack %lld", (long long)getpid());
    flare::base::stop_watcher tm;
    tm.start();
    flare::io::IOBufBuilder pstack_output;
    const int rc = flare::base::read_command_output(pstack_output, cmd.c_str());
    if (rc < 0) {
        LOG(ERROR) << "Fail to popen `" << cmd << "'";
        return;
    }
    pstack_output.move_to(resp);
    tm.stop();
    resp.append(flare::base::string_printf("\n\ntime=%" PRId64 "ms", tm.m_elapsed()));
}

} // namespace brpc
