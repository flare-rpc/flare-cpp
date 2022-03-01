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


#include "flare/log/logging.h"
#include "flare/io/snappy/snappy.h"
#include "flare/rpc/policy/snappy_compress.h"
#include "flare/rpc/protocol.h"


namespace flare::rpc {
namespace policy {

bool SnappyCompress(const google::protobuf::Message& res, flare::cord_buf* buf) {
    flare::cord_buf serialized_pb;
    flare::cord_buf_as_zero_copy_output_stream wrapper(&serialized_pb);
    if (res.SerializeToZeroCopyStream(&wrapper)) {
        flare::cord_buf_as_snappy_source source(serialized_pb);
        flare::cord_buf_as_snappy_sink sink(*buf);
        return flare::snappy::Compress(&source, &sink);
    }
    LOG(WARNING) << "Fail to serialize input pb=" << &res;
    return false;
}

bool SnappyDecompress(const flare::cord_buf& data, google::protobuf::Message* req) {
    flare::cord_buf_as_snappy_source source(data);
    flare::cord_buf binary_pb;
    flare::cord_buf_as_snappy_sink sink(binary_pb);
    if (flare::snappy::Uncompress(&source, &sink)) {
        return ParsePbFromCordBuf(req, binary_pb);
    }
    LOG(WARNING) << "Fail to snappy::Uncompress, size=" << data.size();
    return false;
}

bool SnappyCompress(const flare::cord_buf& in, flare::cord_buf* out) {
    flare::cord_buf_as_snappy_source source(in);
    flare::cord_buf_as_snappy_sink sink(*out);
    return flare::snappy::Compress(&source, &sink);
}

bool SnappyDecompress(const flare::cord_buf& in, flare::cord_buf* out) {
    flare::cord_buf_as_snappy_source source(in);
    flare::cord_buf_as_snappy_sink sink(*out);
    return flare::snappy::Uncompress(&source, &sink);
}

}  // namespace policy
} // namespace flare::rpc
