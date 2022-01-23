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


#include <google/protobuf/descriptor.h>         // MethodDescriptor
#include <google/protobuf/message.h>            // Message
#include <gflags/gflags.h>

#include "flare/base/time.h"
#include "flare/io/iobuf.h"                        // flare::io::IOBuf

#include "flare/brpc/controller.h"               // Controller
#include "flare/brpc/socket.h"                   // Socket
#include "flare/brpc/server.h"                   // Server
#include "flare/brpc/details/server_private_accessor.h"
#include "flare/brpc/span.h"
#include "flare/brpc/errno.pb.h"                 // EREQUEST, ERESPONSE
#include "flare/brpc/details/controller_private_accessor.h"
#include "flare/brpc/policy/most_common_message.h"
#include "flare/brpc/policy/nshead_mcpack_protocol.h"
#include "flare/mcpack2pb/mcpack2pb.h"


namespace brpc {
namespace policy {

void NsheadMcpackAdaptor::ParseNsheadMeta(
    const Server& svr, const NsheadMessage& /*request*/, Controller* cntl,
    NsheadMeta* out_meta) const {
    google::protobuf::Service* service = svr.first_service();
    if (!service) {
        cntl->SetFailed(ENOSERVICE, "No first_service in this server");
        return;
    }
    const google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();
    if (sd->method_count() == 0) {
        cntl->SetFailed(ENOMETHOD, "No method in service=%s",
                        sd->full_name().c_str());
        return;
    }
    const google::protobuf::MethodDescriptor* method = sd->method(0);
    out_meta->set_full_method_name(method->full_name());
}

void NsheadMcpackAdaptor::ParseRequestFromIOBuf(
    const NsheadMeta&, const NsheadMessage& raw_req,
    Controller* cntl, google::protobuf::Message* pb_req) const {
    const std::string& msg_name = pb_req->GetDescriptor()->full_name();
    mcpack2pb::MessageHandler handler = mcpack2pb::find_message_handler(msg_name);
    if (!handler.parse_from_iobuf(pb_req, raw_req.body)) {
        cntl->SetFailed(EREQUEST, "Fail to parse request message, "
                        "request_size=%" PRIu64, (uint64_t)raw_req.body.length());
        return;
    }
}

void NsheadMcpackAdaptor::SerializeResponseToIOBuf(
    const NsheadMeta&, Controller* cntl,
    const google::protobuf::Message* pb_res, NsheadMessage* raw_res) const {
    if (cntl->Failed()) {
        cntl->CloseConnection("Close connection due to previous error");
        return;
    }
    CompressType type = cntl->response_compress_type();
    if (type != COMPRESS_TYPE_NONE) {
        LOG(WARNING) << "nshead_mcpack protocol doesn't support compression";
        type = COMPRESS_TYPE_NONE;
    }
    
    if (pb_res == NULL) {
        cntl->CloseConnection("response was not created yet");
        return;
    }

    const std::string& msg_name = pb_res->GetDescriptor()->full_name();
    mcpack2pb::MessageHandler handler = mcpack2pb::find_message_handler(msg_name);
    if (!handler.serialize_to_iobuf(*pb_res, &raw_res->body,
                                   ::mcpack2pb::FORMAT_MCPACK_V2)) {
        cntl->CloseConnection("Fail to serialize %s", msg_name.c_str());
        return;
    }
}

void ProcessNsheadMcpackResponse(InputMessageBase* msg_base) {
    const int64_t start_parse_us = flare::base::cpuwide_time_us();
    DestroyingPtr<MostCommonMessage> msg(static_cast<MostCommonMessage*>(msg_base));
    const Socket* socket = msg->socket();
    
    // Fetch correlation id that we saved before in `PackNsheadMcpackRequest'
    const bthread_id_t cid = { static_cast<uint64_t>(socket->correlation_id()) };
    Controller* cntl = NULL;
    const int rc = bthread_id_lock(cid, (void**)&cntl);
    if (rc != 0) {
        LOG_IF(ERROR, rc != EINVAL && rc != EPERM)
            << "Fail to lock correlation_id=" << cid << ": " << flare_error(rc);
        return;
    }
    
    ControllerPrivateAccessor accessor(cntl);
    Span* span = accessor.span();
    if (span) {
        span->set_base_real_us(msg->base_real_us());
        span->set_received_us(msg->received_us());
        span->set_response_size(msg->meta.size() + msg->payload.size());
        span->set_start_parse_us(start_parse_us);
    }
    const int saved_error = cntl->ErrorCode();
    google::protobuf::Message* res = cntl->response();
    if (res == NULL) {
        // silently ignore response.
        return;
    }
    const std::string& msg_name = res->GetDescriptor()->full_name();
    mcpack2pb::MessageHandler handler = mcpack2pb::find_message_handler(msg_name);
    if (!handler.parse_from_iobuf(res, msg->payload)) {
        return cntl->CloseConnection("Fail to parse response message");
    }
    // Unlocks correlation_id inside. Revert controller's
    // error code if it version check of `cid' fails
    msg.reset();  // optional, just release resourse ASAP
    accessor.OnResponse(cid, saved_error);
} 

void SerializeNsheadMcpackRequest(flare::io::IOBuf* buf, Controller* cntl,
                          const google::protobuf::Message* pb_req) {
    CompressType type = cntl->request_compress_type();
    if (type != COMPRESS_TYPE_NONE) {
        cntl->SetFailed(EREQUEST,
                        "nshead_mcpack protocol doesn't support compression");
        return;
    }
    const std::string& msg_name = pb_req->GetDescriptor()->full_name();
    mcpack2pb::MessageHandler handler = mcpack2pb::find_message_handler(msg_name);
    if (!handler.serialize_to_iobuf(*pb_req, buf, ::mcpack2pb::FORMAT_MCPACK_V2)) {
        cntl->SetFailed(EREQUEST, "Fail to serialize %s", msg_name.c_str());
        return;
    }
}

void PackNsheadMcpackRequest(flare::io::IOBuf* buf,
                             SocketMessage**,
                             uint64_t correlation_id,
                             const google::protobuf::MethodDescriptor*,
                             Controller* controller,
                             const flare::io::IOBuf& request,
                             const Authenticator* /*not supported*/) {
    ControllerPrivateAccessor accessor(controller);
    if (controller->connection_type() == CONNECTION_TYPE_SINGLE) {
        return controller->SetFailed(
            EINVAL, "nshead_mcpack can't work with CONNECTION_TYPE_SINGLE");
    }
    // Store `correlation_id' into Socket since nshead_mcpack protocol
    // doesn't contain this field
    accessor.get_sending_socket()->set_correlation_id(correlation_id);
        
    nshead_t nshead;
    memset(&nshead, 0, sizeof(nshead_t));
    nshead.log_id = controller->log_id();
    nshead.magic_num = NSHEAD_MAGICNUM;
    nshead.body_len = request.size();
    buf->append(&nshead, sizeof(nshead));

    // Span* span = accessor.span();
    // if (span) {
    //     request_meta->set_trace_id(span->trace_id());
    //     request_meta->set_span_id(span->span_id());
    //     request_meta->set_parent_span_id(span->parent_span_id());
    // }
    buf->append(request);
}

}  // namespace policy
} // namespace brpc
