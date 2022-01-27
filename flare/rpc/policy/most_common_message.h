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


#ifndef BRPC_POLICY_MOST_COMMON_MESSAGE_H
#define BRPC_POLICY_MOST_COMMON_MESSAGE_H

#include "flare/memory/object_pool.h"
#include "flare/rpc/input_messenger.h"


namespace flare::rpc {
namespace policy {

// Try to use this message as the intermediate message between Parse() and
// Process() to maximize usage of ObjectPool<MostCommonMessage>, otherwise
// you have to new the messages or use a separate ObjectPool (which is likely
// to waste more memory)
struct FLARE_CACHELINE_ALIGNMENT MostCommonMessage : public InputMessageBase {
    flare::io::cord_buf meta;
    flare::io::cord_buf payload;
    PipelinedInfo pi;

    inline static MostCommonMessage* Get() {
        return flare::memory::get_object<MostCommonMessage>();
    }

    // @InputMessageBase
    void DestroyImpl() {
        meta.clear();
        payload.clear();
        pi.reset();
        flare::memory::return_object(this);
    }
};

}  // namespace policy
} // namespace flare::rpc


#endif  // BRPC_POLICY_MOST_COMMON_MESSAGE_H