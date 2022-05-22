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


#ifndef FLARE_RPC_TRACEPRINTF_H_
#define FLARE_RPC_TRACEPRINTF_H_

#include "flare/base/profile.h"

// To flare developers: This is a header included by user, don't depend
// on internal structures, use opaque pointers instead.


namespace flare::rpc {

    bool CanAnnotateSpan();

    void AnnotateSpan(const char *fmt, ...);

} // namespace flare::rpc


// Use this macro to print log to /rpcz and tracing system.
// If rpcz is not enabled, arguments to this macro is NOT evaluated, don't
// have (critical) side effects in arguments.
#define TRACEPRINTF(fmt, args...)                                       \
    do {                                                                \
        if (::flare::rpc::CanAnnotateSpan()) {                          \
            ::flare::rpc::AnnotateSpan("[" __FILE__ ":" FLARE_SYMBOLSTR(__LINE__) "] " fmt, ##args);           \
        }                                                               \
    } while (0)

#endif  // FLARE_RPC_TRACEPRINTF_H_
