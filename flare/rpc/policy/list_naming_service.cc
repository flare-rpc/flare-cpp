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


#include <cstdlib>                                   // strtol
#include <string>                                     // std::string
#include <set>                                        // std::set
#include "flare/strings/string_splitter.h"                     // StringSplitter
#include "flare/rpc/log.h"
#include "flare/rpc/policy/list_naming_service.h"
#include "flare/strings/utility.h"


namespace flare::rpc {
    namespace policy {

        // Defined in file_naming_service.cpp
        bool SplitIntoServerAndTag(const std::string_view &line,
                                   std::string_view *server_addr,
                                   std::string_view *tag);

        int ParseServerList(const char *service_name,
                            std::vector<ServerNode> *servers) {
            servers->clear();
            // Sort/unique the inserted vector is faster, but may have a different order
            // of addresses from the file. To make assertions in tests easier, we use
            // set to de-duplicate and keep the order.
            std::set<ServerNode> presence;
            std::string line;

            if (!service_name) {
                FLARE_LOG(FATAL) << "Param[service_name] is NULL";
                return -1;
            }
            for (flare::StringSplitter sp(service_name, ','); sp != NULL; ++sp) {
                line.assign(sp.field(), sp.length());
                std::string_view addr;
                std::string_view tag;
                if (!SplitIntoServerAndTag(line, &addr, &tag)) {
                    continue;
                }
                const_cast<char *>(addr.data())[addr.size()] = '\0'; // safe
                flare::base::end_point point;
                if (str2endpoint(addr.data(), &point) != 0 &&
                    hostname2endpoint(addr.data(), &point) != 0) {
                    FLARE_LOG(ERROR) << "Invalid address=`" << addr << '\'';
                    continue;
                }
                ServerNode node;
                node.addr = point;
                flare::copy_to_string(tag, &node.tag);
                if (presence.insert(node).second) {
                    servers->push_back(node);
                } else {
                    RPC_VLOG << "Duplicated server=" << node;
                }
            }
            RPC_VLOG << "Got " << servers->size()
                     << (servers->size() > 1 ? " servers" : " server");
            return 0;
        }

        int ListNamingService::GetServers(const char *service_name,
                                          std::vector<ServerNode> *servers) {
            return ParseServerList(service_name, servers);
        }

        int ListNamingService::RunNamingService(const char *service_name,
                                                NamingServiceActions *actions) {
            std::vector<ServerNode> servers;
            const int rc = GetServers(service_name, &servers);
            if (rc != 0) {
                servers.clear();
            }
            actions->ResetServers(servers);
            return 0;
        }

        void ListNamingService::Describe(
                std::ostream &os, const DescribeOptions &) const {
            os << "list";
            return;
        }

        NamingService *ListNamingService::New() const {
            return new ListNamingService;
        }

        void ListNamingService::Destroy() {
            delete this;
        }

        int DomainListNamingService::GetServers(const char *service_name,
                                                std::vector<ServerNode> *servers) {
            return ParseServerList(service_name, servers);
        }

        void DomainListNamingService::Describe(std::ostream &os,
                                               const DescribeOptions &) const {
            os << "dlist";
            return;
        }

        NamingService *DomainListNamingService::New() const {
            return new DomainListNamingService;
        }

        void DomainListNamingService::Destroy() { delete this; }

    }  // namespace policy
} // namespace flare::rpc
