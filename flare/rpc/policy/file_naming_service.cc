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


#include <stdio.h>                                      // getline
#include <string>                                       // std::string
#include <set>                                          // std::set
#include "flare/files/file_watcher.h"                    // file_watcher
#include "flare/files/readline_file.h"
#include "flare/fiber/internal/fiber.h"                            // flare::fiber_sleep_for
#include "flare/rpc/log.h"
#include "flare/rpc/policy/file_naming_service.h"
#include "flare/strings/utility.h"
#include "flare/fiber/this_fiber.h"


namespace flare::rpc {
    namespace policy {

        bool SplitIntoServerAndTag(const std::string_view &line,
                                   std::string_view *server_addr,
                                   std::string_view *tag) {
            size_t i = 0;
            for (; i < line.size() && isspace(line[i]); ++i) {}
            if (i == line.size() || line[i] == '#') {  // blank line or comments
                return false;
            }
            const char *const addr_start = line.data() + i;
            const char *tag_start = nullptr;
            ssize_t tag_size = 0;
            for (; i < line.size() && !isspace(line[i]); ++i) {}
            if (server_addr) {
                *server_addr = std::string_view(addr_start, line.data() + i - addr_start);
            }
            if (i != line.size()) {
                for (++i; i < line.size() && isspace(line[i]); ++i) {}
                if (i < line.size()) {
                    tag_start = line.data() + i;
                    tag_size = 1;
                    // find start of comments.
                    for (++i; i < line.size() && line[i] != '#'; ++i, ++tag_size) {}
                    // trim ending blanks
                    for (; tag_size > 0 && isspace(tag_start[tag_size - 1]);
                           --tag_size) {}
                }
                if (tag) {
                    if (tag_size) {
                        *tag = std::string_view(tag_start, tag_size);
                    } else {
                        *tag = std::string_view();
                    }
                }
            }
            return true;
        }

        int FileNamingService::GetServers(const char *service_name,
                                          std::vector<ServerNode> *servers) {
            servers->clear();
            // Sort/unique the inserted vector is faster, but may have a different order
            // of addresses from the file. To make assertions in tests easier, we use
            // set to de-duplicate and keep the order.
            std::set < ServerNode > presence;

            flare::readline_file file;
            auto status = file.open(service_name);
            if (!status.is_ok()) {
                FLARE_PLOG(ERROR) << "Fail to open `" << service_name << "'";
                return errno;
            }
            auto &lines = file.lines();
            for (auto line : lines) {
                line = strip_suffix(line, "\n");
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

        int FileNamingService::RunNamingService(const char *service_name,
                                                NamingServiceActions *actions) {
            std::vector<ServerNode> servers;
            flare::file_watcher fw;
            if (fw.init(service_name) < 0) {
                FLARE_LOG(ERROR) << "Fail to init file_watcher on `" << service_name << "'";
                return -1;
            }
            for (;;) {
                const int rc = GetServers(service_name, &servers);
                if (rc != 0) {
                    return rc;
                }
                actions->ResetServers(servers);

                for (;;) {
                    flare::file_watcher::Change change = fw.check_and_consume();
                    if (change > 0) {
                        break;
                    }
                    if (change < 0) {
                        FLARE_LOG(ERROR) << "`" << service_name << "' was deleted";
                    }
                    if (flare::fiber_sleep_for(100000L/*100ms*/) < 0) {
                        if (errno == ESTOP) {
                            return 0;
                        }
                        FLARE_PLOG(ERROR) << "Fail to sleep";
                        return -1;
                    }
                }
            }
            FLARE_CHECK(false);
            return -1;
        }

        void FileNamingService::Describe(std::ostream &os,
                                         const DescribeOptions &) const {
            os << "file";
            return;
        }

        NamingService *FileNamingService::New() const {
            return new FileNamingService;
        }

        void FileNamingService::Destroy() {
            delete this;
        }

    }  // namespace policy
} // namespace flare::rpc
