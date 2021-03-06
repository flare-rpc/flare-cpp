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


#ifndef FLARE_RPC_TRACKME_H_
#define FLARE_RPC_TRACKME_H_

// [Internal] RPC users are not supposed to call functions below. 

#include "flare/base/endpoint.h"


namespace flare::rpc {

    // Set the server address for reporting.
    // Currently only the first address will be saved.
    void SetTrackMeAddress(flare::base::end_point pt);

    // Call this function every second (or every several seconds) to send
    // TrackMeRequest to -trackme_server every TRACKME_INTERVAL seconds.
    void TrackMe();

} // namespace flare::rpc


#endif // FLARE_RPC_TRACKME_H_
