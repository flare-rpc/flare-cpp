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

#ifndef FLARE_RPC_ESP_MESSAGE_H_
#define FLARE_RPC_ESP_MESSAGE_H_

#include <string>

#include <google/protobuf/message.h>
#include <google/protobuf/generated_message_reflection.h>   // dynamic_cast_if_available
#include <google/protobuf/reflection_ops.h>     // ReflectionOps::Merge

#include "flare/rpc/esp_head.h"
#include "flare/io/cord_buf.h"
#include "flare/rpc/proto_base.pb.h"

namespace flare::rpc {

    class EspMessage : public ::google::protobuf::Message {
    public:
        EspHead head;
        flare::cord_buf body;

    public:
        EspMessage();

        virtual ~EspMessage();

        EspMessage(const EspMessage &from);

        inline EspMessage &operator=(const EspMessage &from) {
            CopyFrom(from);
            return *this;
        }

        static const ::google::protobuf::Descriptor *descriptor();

        static const EspMessage &default_instance();

        void Swap(EspMessage *other);

        // implements Message ----------------------------------------------

        EspMessage *New() const override;

#if GOOGLE_PROTOBUF_VERSION >= 3006000

        EspMessage *New(::google::protobuf::Arena *arena) const override;

#endif

        void CopyFrom(const ::google::protobuf::Message &from) override;

        void MergeFrom(const ::google::protobuf::Message &from) override;

        void CopyFrom(const EspMessage &from);

        void MergeFrom(const EspMessage &from);

        void Clear() override;

        bool IsInitialized() const override;

        int ByteSize() const;

        bool MergePartialFromCodedStream(
                ::google::protobuf::io::CodedInputStream *input);

        void SerializeWithCachedSizes(
                ::google::protobuf::io::CodedOutputStream *output) const;

        ::google::protobuf::uint8 *SerializeWithCachedSizesToArray(
                ::google::protobuf::uint8 *output) const;

        int GetCachedSize() const override { return ByteSize(); }

    protected:
        ::google::protobuf::Metadata GetMetadata() const override;

    private:
        void SharedCtor();

        void SharedDtor();
    };

} // namespace flare::rpc

#endif  // FLARE_RPC_ESP_MESSAGE_H_
