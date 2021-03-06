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

// mcpack2pb - Make protobuf be front-end of mcpack/compack

// Date: Mon Oct 19 17:17:36 CST 2015

#ifndef MCPACK2PB_MCPACK_MCPACK2PB_H
#define MCPACK2PB_MCPACK_MCPACK2PB_H

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "flare/container/flat_map.h"
#include "flare/io/cord_buf.h"
#include "flare/mcpack2pb/parser.h"
#include "flare/mcpack2pb/serializer.h"

namespace mcpack2pb {

typedef bool (*SetFieldFn)(::google::protobuf::Message* msg,
                           UnparsedValue& value);

// Mapping from filed name to its parsing&setting function.
typedef flare::container::FlatMap<std::string_view, SetFieldFn> FieldMap;

enum SerializationFormat {
    FORMAT_COMPACK,
    FORMAT_MCPACK_V2
};

struct MessageHandler {
    // Parse `msg' from `input' as a mcpack_v2 or compack object.
    // Returns number of bytes consumed.
    size_t (*parse)(::google::protobuf::Message* msg,
                    ::google::protobuf::io::ZeroCopyInputStream* input);

    // Parse `msg' from `input' as a mcpack_v2 or compack object removed with header.
    // Returns true on success.
    bool (*parse_body)(::google::protobuf::Message* msg,
                       ::google::protobuf::io::ZeroCopyInputStream* input,
                       size_t size);
    
    // Serialize `msg' as a mcpack_v2 or compack object into `output'.
    // The serialization format is decided by `format'.
    // Returns true on success.
    bool (*serialize)(const ::google::protobuf::Message& msg,
                      ::google::protobuf::io::ZeroCopyOutputStream* output,
                      SerializationFormat format);

    // Serialize `msg' as a mcpack_v2 or compack object without header into
    // `serializer'. The serialization format is decided by `format'.
    // Returns true on success.
    void (*serialize_body)(const ::google::protobuf::Message& msg,
                           Serializer& serializer,
                           SerializationFormat format);

    // -------------------
    //  Helper functions
    // -------------------

    // Parse `msg' from cord_buf or array which may contain more data than just
    // the message.
    // Returns bytes parsed, 0 on error.
    size_t parse_from_iobuf_prefix(::google::protobuf::Message* msg,
                                   const ::flare::cord_buf& buf);
    size_t parse_from_array_prefix(::google::protobuf::Message* msg,
                                   const void* data, int size);
    // Parse `msg' from cord_buf or array which may just contain the message.
    // Returns true on success.
    bool parse_from_iobuf(::google::protobuf::Message* msg,
                          const ::flare::cord_buf& buf);
    bool parse_from_array(::google::protobuf::Message* msg,
                          const void* data, int size);
    // Serialize `msg' to cord_buf or string.
    // Returns true on success.
    bool serialize_to_iobuf(const ::google::protobuf::Message& msg,
                            ::flare::cord_buf* buf, SerializationFormat format);

    // TODO(gejun): serialize_to_string is not supported because OutputStream
    // requires the embedded zero-copy stream to return permanent memory blocks
    // to support reserve() however the string inside StringOutputStream may
    // be resized and invalidates previous returned memory blocks.
};

static const MessageHandler INVALID_MESSAGE_HANDLER = {NULL, NULL, NULL, NULL};

// if the *.pb.cc and *.pb.h was generated by mcpack2pb. This function will be
// called with the mcpack/compack parser and serializer BEFORE main().
void register_message_handler_or_die(const std::string& full_name,
                                     const MessageHandler& handler);

// Find the registered parser/serializer by `full_name'
// e.g. "example.SampleMessage".
// If the handler was not registered, function pointers inside are NULL.
MessageHandler find_message_handler(const std::string& full_name);

// inline impl.
inline size_t MessageHandler::parse_from_iobuf_prefix(
    ::google::protobuf::Message* msg, const ::flare::cord_buf& buf) {
    if (parse == NULL) {
        FLARE_LOG(ERROR) << "`parse' is NULL";
        return 0;
    }
    ::flare::cord_buf_as_zero_copy_input_stream zc_stream(buf);
    return parse(msg, &zc_stream);
}

inline bool MessageHandler::parse_from_iobuf(
    ::google::protobuf::Message* msg, const ::flare::cord_buf& buf) {
    if (parse == NULL) {
        FLARE_LOG(ERROR) << "`parse' is NULL";
        return 0;
    }
    ::flare::cord_buf_as_zero_copy_input_stream zc_stream(buf);
    return parse(msg, &zc_stream) == buf.size();
}

inline size_t MessageHandler::parse_from_array_prefix(
    ::google::protobuf::Message* msg, const void* data, int size) {
    if (parse == NULL) {
        FLARE_LOG(ERROR) << "`parse' is NULL";
        return 0;
    }
    ::google::protobuf::io::ArrayInputStream zc_stream(data, size);
    return parse(msg, &zc_stream);
}

inline bool MessageHandler::parse_from_array(
    ::google::protobuf::Message* msg, const void* data, int size) {
    if (parse == NULL) {
        FLARE_LOG(ERROR) << "`parse' is NULL";
        return 0;
    }
    ::google::protobuf::io::ArrayInputStream zc_stream(data, size);
    return (int)parse(msg, &zc_stream) == size;
}

inline bool MessageHandler::serialize_to_iobuf(
    const ::google::protobuf::Message& msg,
    ::flare::cord_buf* buf, SerializationFormat format) {
    if (serialize == NULL) {
        FLARE_LOG(ERROR) << "`serialize' is NULL";
        return false;
    }
    ::flare::cord_buf_as_zero_copy_output_stream zc_stream(buf);
    return serialize(msg, &zc_stream, format);
}

} // namespace mcpack2pb

#endif // MCPACK2PB_MCPACK_MCPACK2PB_H
