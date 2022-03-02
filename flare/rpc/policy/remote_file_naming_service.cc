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


#include <gflags/gflags.h>
#include <stdio.h>                                      // getline
#include <string>                                       // std::string
#include <set>                                          // std::set
#include "flare/fiber/internal/fiber.h"                            // flare::fiber_sleep_for
#include "flare/io/cord_buf.h"
#include "flare/rpc/log.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/policy/remote_file_naming_service.h"
#include "flare/strings/utility.h"
#include "flare/strings/safe_substr.h"

namespace flare::rpc {
    namespace policy {

        DEFINE_int32(remote_file_connect_timeout_ms, -1,
                     "Timeout for creating connections to fetch remote server lists,"
                     " set to remote_file_timeout_ms/3 by default (-1)");
        DEFINE_int32(remote_file_timeout_ms, 1000,
                     "Timeout for fetching remote server lists");

// Defined in file_naming_service.cpp
        bool SplitIntoServerAndTag(const std::string_view &line,
                                   std::string_view *server_addr,
                                   std::string_view *tag);

        static bool CutLineFromCordBuf(flare::cord_buf *source, std::string *line_out) {
            if (source->empty()) {
                return false;
            }
            flare::cord_buf line_data;
            if (source->cut_until(&line_data, "\n") != 0) {
                source->cutn(line_out, source->size());
                return true;
            }
            line_data.copy_to(line_out);
            if (!line_out->empty() && flare::back_char(*line_out) == '\r') {
                line_out->resize(line_out->size() - 1);
            }
            return true;
        }

        int RemoteFileNamingService::GetServers(const char *service_name_cstr,
                                                std::vector<ServerNode> *servers) {
            servers->clear();

            if (_channel == NULL) {
                std::string_view tmpname(service_name_cstr);
                size_t pos = tmpname.find("://");
                std::string_view proto;
                if (pos != std::string_view::npos) {
                    proto =flare::safe_substr(tmpname, 0, pos);
                    for (pos += 3; tmpname[pos] == '/'; ++pos) {}
                    tmpname.remove_prefix(pos);
                } else {
                    proto = "http";
                }
                if (proto != "bns" && proto != "http") {
                    LOG(ERROR) << "Invalid protocol=`" << proto
                               << "\' in service_name=" << service_name_cstr;
                    return -1;
                }
                size_t slash_pos = tmpname.find('/');
                std::string_view server_addr_piece;
                if (slash_pos == std::string_view::npos) {
                    server_addr_piece = tmpname;
                    _path = "/";
                } else {
                    server_addr_piece =flare::safe_substr(tmpname, 0, slash_pos);
                    _path = flare::as_string(flare::safe_substr(tmpname, slash_pos));
                }
                _server_addr.reserve(proto.size() + 3 + server_addr_piece.size());
                _server_addr.append(proto.data(), proto.size());
                _server_addr.append("://");
                _server_addr.append(server_addr_piece.data(), server_addr_piece.size());
                ChannelOptions opt;
                opt.protocol = PROTOCOL_HTTP;
                opt.connect_timeout_ms = FLAGS_remote_file_connect_timeout_ms > 0 ?
                                         FLAGS_remote_file_connect_timeout_ms : FLAGS_remote_file_timeout_ms / 3;
                opt.timeout_ms = FLAGS_remote_file_timeout_ms;
                std::unique_ptr<Channel> chan(new Channel);
                if (chan->Init(_server_addr.c_str(), "rr", &opt) != 0) {
                    LOG(ERROR) << "Fail to init channel to " << _server_addr;
                    return -1;
                }
                _channel.swap(chan);
            }

            Controller cntl;
            cntl.http_request().uri() = _path;
            _channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
            if (cntl.Failed()) {
                LOG(WARNING) << "Fail to access " << _server_addr << _path << ": "
                             << cntl.ErrorText();
                return -1;
            }
            std::string line;
            // Sort/unique the inserted vector is faster, but may have a different order
            // of addresses from the file. To make assertions in tests easier, we use
            // set to de-duplicate and keep the order.
            std::set < ServerNode > presence;

            while (CutLineFromCordBuf(&cntl.response_attachment(), &line)) {
                std::string_view addr;
                std::string_view tag;
                if (!SplitIntoServerAndTag(line, &addr, &tag)) {
                    continue;
                }
                const_cast<char *>(addr.data())[addr.size()] = '\0'; // safe
                flare::base::end_point point;
                if (str2endpoint(addr.data(), &point) != 0 &&
                    hostname2endpoint(addr.data(), &point) != 0) {
                    LOG(ERROR) << "Invalid address=`" << addr << '\'';
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
                     << (servers->size() > 1 ? " servers" : " server")
                     << " from " << service_name_cstr;
            return 0;
        }

        void RemoteFileNamingService::Describe(std::ostream &os,
                                               const DescribeOptions &) const {
            os << "remotefile";
            return;
        }

        NamingService *RemoteFileNamingService::New() const {
            return new RemoteFileNamingService;
        }

        void RemoteFileNamingService::Destroy() {
            delete this;
        }

    }  // namespace policy
} // namespace flare::rpc
