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
#include "flare/butil/files/file_watcher.h"                    // FileWatcher
#include "flare/butil/files/scoped_file.h"                     // ScopedFILE
#include "flare/bthread/bthread.h"                            // bthread_usleep
#include "flare/brpc/log.h"
#include "flare/brpc/policy/file_naming_service.h"
#include "flare/base/strings.h"


namespace brpc {
namespace policy {

bool SplitIntoServerAndTag(const std::string_view& line,
                           std::string_view* server_addr,
                           std::string_view* tag) {
    size_t i = 0;
    for (; i < line.size() && isspace(line[i]); ++i) {}
    if (i == line.size() || line[i] == '#') {  // blank line or comments
        return false;
    }
    const char* const addr_start = line.data() + i;
    const char* tag_start = NULL;
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
                                  std::vector<ServerNode>* servers) {
    servers->clear();
    char* line = NULL;
    size_t line_len = 0;
    ssize_t nr = 0;
    // Sort/unique the inserted vector is faster, but may have a different order
    // of addresses from the file. To make assertions in tests easier, we use
    // set to de-duplicate and keep the order.
    std::set<ServerNode> presence;

    butil::ScopedFILE fp(fopen(service_name, "r"));
    if (!fp) {
        PLOG(ERROR) << "Fail to open `" << service_name << "'";
        return errno;
    }
    while ((nr = getline(&line, &line_len, fp.get())) != -1) {
        if (line[nr - 1] == '\n') { // remove ending newline
            --nr;
        }
        std::string_view addr;
        std::string_view tag;
        if (!SplitIntoServerAndTag(std::string_view(line, nr),
                                   &addr, &tag)) {
            continue;
        }
        const_cast<char*>(addr.data())[addr.size()] = '\0'; // safe
        flare::base::end_point point;
        if (str2endpoint(addr.data(), &point) != 0 &&
            hostname2endpoint(addr.data(), &point) != 0) {
            LOG(ERROR) << "Invalid address=`" << addr << '\'';
            continue;
        }
        ServerNode node;
        node.addr = point;
        flare::base::copy_to_string(tag, &node.tag);
        if (presence.insert(node).second) {
            servers->push_back(node);
        } else {
            RPC_VLOG << "Duplicated server=" << node;
        }
    }
    RPC_VLOG << "Got " << servers->size()
             << (servers->size() > 1 ? " servers" : " server");
    free(line);
    return 0;
}

int FileNamingService::RunNamingService(const char* service_name,
                                        NamingServiceActions* actions) {
    std::vector<ServerNode> servers;
    butil::FileWatcher fw;
    if (fw.init(service_name) < 0) {
        LOG(ERROR) << "Fail to init FileWatcher on `" << service_name << "'";
        return -1;
    }
    for (;;) {
        const int rc = GetServers(service_name, &servers);
        if (rc != 0) {
            return rc;
        }
        actions->ResetServers(servers);

        for (;;) {
            butil::FileWatcher::Change change = fw.check_and_consume();
            if (change > 0) {
                break;
            }
            if (change < 0) {
                LOG(ERROR) << "`" << service_name << "' was deleted";
            }
            if (bthread_usleep(100000L/*100ms*/) < 0) {
                if (errno == ESTOP) {
                    return 0;
                }
                PLOG(ERROR) << "Fail to sleep";
                return -1;
            }
        }
    }
    CHECK(false);
    return -1;
}

void FileNamingService::Describe(std::ostream& os,
                                 const DescribeOptions&) const {
    os << "file";
    return;
}

NamingService* FileNamingService::New() const {
    return new FileNamingService;
}

void FileNamingService::Destroy() {
    delete this;
}

}  // namespace policy
} // namespace brpc
