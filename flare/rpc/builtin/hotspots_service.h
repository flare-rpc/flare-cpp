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


#ifndef FLARE_RPC_HOTSPOTS_SERVICE_H_
#define FLARE_RPC_HOTSPOTS_SERVICE_H_

#include "flare/rpc/builtin/common.h"
#include "flare/rpc/builtin_service.pb.h"
#include "flare/rpc/builtin/tabbed.h"


namespace flare::rpc {

    class Server;

    class HotspotsService : public hotspots, public Tabbed {
    public:
        void cpu(::google::protobuf::RpcController *cntl_base,
                 const ::flare::rpc::HotspotsRequest *request,
                 ::flare::rpc::HotspotsResponse *response,
                 ::google::protobuf::Closure *done);

        void heap(::google::protobuf::RpcController *cntl_base,
                  const ::flare::rpc::HotspotsRequest *request,
                  ::flare::rpc::HotspotsResponse *response,
                  ::google::protobuf::Closure *done);

        void growth(::google::protobuf::RpcController *cntl_base,
                    const ::flare::rpc::HotspotsRequest *request,
                    ::flare::rpc::HotspotsResponse *response,
                    ::google::protobuf::Closure *done);

        void contention(::google::protobuf::RpcController *cntl_base,
                        const ::flare::rpc::HotspotsRequest *request,
                        ::flare::rpc::HotspotsResponse *response,
                        ::google::protobuf::Closure *done);

        void cpu_non_responsive(::google::protobuf::RpcController *cntl_base,
                                const ::flare::rpc::HotspotsRequest *request,
                                ::flare::rpc::HotspotsResponse *response,
                                ::google::protobuf::Closure *done);

        void heap_non_responsive(::google::protobuf::RpcController *cntl_base,
                                 const ::flare::rpc::HotspotsRequest *request,
                                 ::flare::rpc::HotspotsResponse *response,
                                 ::google::protobuf::Closure *done);

        void growth_non_responsive(::google::protobuf::RpcController *cntl_base,
                                   const ::flare::rpc::HotspotsRequest *request,
                                   ::flare::rpc::HotspotsResponse *response,
                                   ::google::protobuf::Closure *done);

        void contention_non_responsive(::google::protobuf::RpcController *cntl_base,
                                       const ::flare::rpc::HotspotsRequest *request,
                                       ::flare::rpc::HotspotsResponse *response,
                                       ::google::protobuf::Closure *done);

        void GetTabInfo(flare::rpc::TabInfoList *) const;
    };

} // namespace flare::rpc



#endif // FLARE_RPC_HOTSPOTS_SERVICE_H_
