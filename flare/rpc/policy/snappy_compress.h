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


#ifndef BRPC_POLICY_SNAPPY_COMPRESS_H
#define BRPC_POLICY_SNAPPY_COMPRESS_H

#include <google/protobuf/message.h>          // Message
#include "flare/io/iobuf.h"                       // cord_buf


namespace flare::rpc {
namespace policy {

// Compress serialized `msg' into `buf'.
bool SnappyCompress(const google::protobuf::Message& msg, flare::io::cord_buf* buf);

// Parse `msg' from decompressed `buf'
bool SnappyDecompress(const flare::io::cord_buf& data, google::protobuf::Message* msg);

// Put compressed `in' into `out'.
bool SnappyCompress(const flare::io::cord_buf& in, flare::io::cord_buf* out);

// Put decompressed `in' into `out'.
bool SnappyDecompress(const flare::io::cord_buf& in, flare::io::cord_buf* out);

}  // namespace policy
} // namespace flare::rpc


#endif // BRPC_POLICY_SNAPPY_COMPRESS_H
