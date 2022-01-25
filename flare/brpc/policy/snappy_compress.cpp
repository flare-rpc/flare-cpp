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


#include "flare/base/logging.h"
#include "flare/io/snappy/snappy.h"
#include "flare/brpc/policy/snappy_compress.h"
#include "flare/brpc/protocol.h"


namespace brpc {
namespace policy {

bool SnappyCompress(const google::protobuf::Message& res, flare::io::IOBuf* buf) {
    flare::io::IOBuf serialized_pb;
    flare::io::IOBufAsZeroCopyOutputStream wrapper(&serialized_pb);
    if (res.SerializeToZeroCopyStream(&wrapper)) {
        flare::io::IOBufAsSnappySource source(serialized_pb);
        flare::io::IOBufAsSnappySink sink(*buf);
        return flare::snappy::Compress(&source, &sink);
    }
    LOG(WARNING) << "Fail to serialize input pb=" << &res;
    return false;
}

bool SnappyDecompress(const flare::io::IOBuf& data, google::protobuf::Message* req) {
    flare::io::IOBufAsSnappySource source(data);
    flare::io::IOBuf binary_pb;
    flare::io::IOBufAsSnappySink sink(binary_pb);
    if (flare::snappy::Uncompress(&source, &sink)) {
        return ParsePbFromIOBuf(req, binary_pb);
    }
    LOG(WARNING) << "Fail to snappy::Uncompress, size=" << data.size();
    return false;
}

bool SnappyCompress(const flare::io::IOBuf& in, flare::io::IOBuf* out) {
    flare::io::IOBufAsSnappySource source(in);
    flare::io::IOBufAsSnappySink sink(*out);
    return flare::snappy::Compress(&source, &sink);
}

bool SnappyDecompress(const flare::io::IOBuf& in, flare::io::IOBuf* out) {
    flare::io::IOBufAsSnappySource source(in);
    flare::io::IOBufAsSnappySink sink(*out);
    return flare::snappy::Uncompress(&source, &sink);
}

}  // namespace policy
} // namespace brpc
