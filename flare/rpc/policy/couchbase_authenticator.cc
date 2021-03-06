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

#include "flare/rpc/policy/couchbase_authenticator.h"

#include "flare/base/base64.h"
#include "flare/io/cord_buf.h"
#include "flare/strings/str_format.h"
#include "flare/base/endian.h"
#include "flare/rpc/policy/memcache_binary_header.h"

namespace flare::rpc {
    namespace policy {

        namespace {

            constexpr char kPlainAuthCommand[] = "PLAIN";
            constexpr char kPadding[1] = {'\0'};

        }  // namespace

        // To get the couchbase authentication protocol, see
        // https://developer.couchbase.com/documentation/server/3.x/developer/dev-guide-3.0/sasl.html
        int CouchbaseAuthenticator::GenerateCredential(std::string *auth_str) const {
            const flare::rpc::policy::MemcacheRequestHeader header = {
                    flare::rpc::policy::MC_MAGIC_REQUEST, flare::rpc::policy::MC_BINARY_SASL_AUTH,
                    flare::base::flare_hton16(sizeof(kPlainAuthCommand) - 1), 0, 0, 0,
                    flare::base::flare_hton32(sizeof(kPlainAuthCommand) + 1 +
                                              bucket_name_.length() * 2 + bucket_password_.length()),
                    0, 0};
            auth_str->clear();
            auth_str->append(reinterpret_cast<const char *>(&header), sizeof(header));
            auth_str->append(kPlainAuthCommand, sizeof(kPlainAuthCommand) - 1);
            auth_str->append(bucket_name_);
            auth_str->append(kPadding, sizeof(kPadding));
            auth_str->append(bucket_name_);
            auth_str->append(kPadding, sizeof(kPadding));
            auth_str->append(bucket_password_);
            return 0;
        }

    }  // namespace policy
}  // namespace flare::rpc
