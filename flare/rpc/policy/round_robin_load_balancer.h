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


#ifndef FLARE_RPC_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_
#define FLARE_RPC_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_

#include <vector>                                      // std::vector
#include <map>                                         // std::map
#include "flare/container/doubly_buffered_data.h"
#include "flare/rpc/load_balancer.h"
#include "flare/rpc/cluster_recover_policy.h"

namespace flare::rpc {
    namespace policy {

        // This LoadBalancer selects server evenly. Selected numbers of servers(added
        // at the same time) are very close.
        class RoundRobinLoadBalancer : public LoadBalancer {
        public:
            bool AddServer(const ServerId &id);

            bool RemoveServer(const ServerId &id);

            size_t AddServersInBatch(const std::vector<ServerId> &servers);

            size_t RemoveServersInBatch(const std::vector<ServerId> &servers);

            int SelectServer(const SelectIn &in, SelectOut *out);

            RoundRobinLoadBalancer *New(const std::string_view &) const;

            void Destroy();

            void Describe(std::ostream &, const DescribeOptions &options);

        private:
            struct Servers {
                std::vector<ServerId> server_list;
                std::map<ServerId, size_t> server_map;
            };

            struct TLS {
                TLS() : stride(0), offset(0) {}

                uint32_t stride;
                uint32_t offset;
            };

            bool SetParameters(const std::string_view &params);

            static bool Add(Servers &bg, const ServerId &id);

            static bool Remove(Servers &bg, const ServerId &id);

            static size_t BatchAdd(Servers &bg, const std::vector<ServerId> &servers);

            static size_t BatchRemove(Servers &bg, const std::vector<ServerId> &servers);

            flare::container::DoublyBufferedData<Servers, TLS> _db_servers;
            std::shared_ptr<ClusterRecoverPolicy> _cluster_recover_policy;
        };

    }  // namespace policy
} // namespace flare::rpc


#endif  // FLARE_RPC_POLICY_ROUND_ROBIN_LOAD_BALANCER_H_
