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


#ifndef FLARE_RPC_BUILTIN_VIZ_MIN_JS_H_
#define FLARE_RPC_BUILTIN_VIZ_MIN_JS_H_

#include "flare/io/cord_buf.h"


namespace flare::rpc {

    // Get the viz.min.js as string or cord_buf.
    // We need to pack all js inside C++ code so that builtin services can be
    // accessed without external resources and network connection.
    const char *viz_min_js();

    const flare::cord_buf &viz_min_js_iobuf();

    const flare::cord_buf &viz_min_js_iobuf_gzip();

} // namespace flare::rpc


#endif // FLARE_RPC_BUILTIN_VIZ_MIN_JS_H_
