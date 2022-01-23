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

#include "flare/brpc/policy/redis_authenticator.h"

#include "flare/base/base64.h"
#include "flare/io/iobuf.h"
#include "flare/base/strings.h"
#include "flare/butil/sys_byteorder.h"
#include "flare/brpc/redis_command.h"

namespace brpc {
namespace policy {

int RedisAuthenticator::GenerateCredential(std::string* auth_str) const {
    flare::io::IOBuf buf;
    brpc::RedisCommandFormat(&buf, "AUTH %s", passwd_.c_str());
    *auth_str = buf.to_string();
    return 0;
}

}  // namespace policy
}  // namespace brpc
