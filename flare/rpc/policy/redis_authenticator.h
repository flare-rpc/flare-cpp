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

#ifndef FLARE_RPC_POLICY_REDIS_AUTHENTICATOR_H_
#define FLARE_RPC_POLICY_REDIS_AUTHENTICATOR_H_

#include "flare/rpc/authenticator.h"

namespace flare::rpc {
    namespace policy {

        // Request to redis for authentication.
        class RedisAuthenticator : public Authenticator {
        public:
            RedisAuthenticator(const std::string &passwd)
                    : passwd_(passwd) {}

            int GenerateCredential(std::string *auth_str) const;

            int VerifyCredential(const std::string &, const flare::base::end_point &,
                                 flare::rpc::AuthContext *) const {
                return 0;
            }

        private:
            const std::string passwd_;
        };

    }  // namespace policy
}  // namespace flare::rpc

#endif  // FLARE_RPC_POLICY_COUCHBASE_AUTHENTICATOR_H_
