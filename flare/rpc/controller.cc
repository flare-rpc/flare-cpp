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


#include <signal.h>
#include <inttypes.h>
#include <openssl/md5.h>
#include <google/protobuf/descriptor.h>
#include <gflags/gflags.h>
#include "flare/fiber/internal/fiber.h"
#include "flare/strings/str_format.h"
#include "flare/log/logging.h"
#include "flare/times/time.h"
#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/unstable.h"
#include "flare/metrics/all.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/socket_map.h"
#include "flare/rpc/channel.h"
#include "flare/rpc/load_balancer.h"
#include "flare/rpc/closure_guard.h"
#include "flare/rpc/details/controller_private_accessor.h"
#include "flare/rpc/controller.h"
#include "flare/rpc/span.h"
#include "flare/rpc/server.h"   // Server::_session_local_data_pool
#include "flare/rpc/simple_data_pool.h"
#include "flare/rpc/retry_policy.h"
#include "flare/rpc/stream_impl.h"
#include "flare/rpc/policy/streaming_rpc_protocol.h" // FIXME
#include "flare/rpc/rpc_dump.h"
#include "flare/rpc/details/usercode_backup_pool.h"  // RunUserCode
#include "flare/rpc/mongo_service_adaptor.h"

// Force linking the .o in UT (which analysis deps by inclusions)
#include "flare/rpc/parallel_channel.h"
#include "flare/rpc/selective_channel.h"

// This is the only place that both client/server must link, so we put
// registrations of errno here.
FLARE_REGISTER_ERRNO(flare::rpc::ENOSERVICE, "No such service");
FLARE_REGISTER_ERRNO(flare::rpc::ENOMETHOD, "No such method");
FLARE_REGISTER_ERRNO(flare::rpc::EREQUEST, "Bad request");
FLARE_REGISTER_ERRNO(flare::rpc::ERPCAUTH, "Authentication failed");
FLARE_REGISTER_ERRNO(flare::rpc::ETOOMANYFAILS, "Too many sub channels failed");
FLARE_REGISTER_ERRNO(flare::rpc::EPCHANFINISH, "ParallelChannel finished");
FLARE_REGISTER_ERRNO(flare::rpc::EBACKUPREQUEST, "Sending backup request");
FLARE_REGISTER_ERRNO(flare::rpc::ERPCTIMEDOUT, "RPC call is timed out");
FLARE_REGISTER_ERRNO(flare::rpc::EFAILEDSOCKET, "Broken socket");
FLARE_REGISTER_ERRNO(flare::rpc::EHTTP, "Bad http call");
FLARE_REGISTER_ERRNO(flare::rpc::EOVERCROWDED, "The server is overcrowded");
FLARE_REGISTER_ERRNO(flare::rpc::ERTMPPUBLISHABLE, "RtmpRetryingClientStream is publishable");
FLARE_REGISTER_ERRNO(flare::rpc::ERTMPCREATESTREAM, "createStream was rejected by the RTMP server");
FLARE_REGISTER_ERRNO(flare::rpc::EEOF, "Got EOF");
FLARE_REGISTER_ERRNO(flare::rpc::EUNUSED, "The socket was not needed");
FLARE_REGISTER_ERRNO(flare::rpc::ESSL, "SSL related operation failed");
FLARE_REGISTER_ERRNO(flare::rpc::EH2RUNOUTSTREAMS, "The H2 socket was run out of streams");

FLARE_REGISTER_ERRNO(flare::rpc::EINTERNAL, "General internal error");
FLARE_REGISTER_ERRNO(flare::rpc::ERESPONSE, "Bad response");
FLARE_REGISTER_ERRNO(flare::rpc::ELOGOFF, "Server is stopping");
FLARE_REGISTER_ERRNO(flare::rpc::ELIMIT, "Reached server's max_concurrency");
FLARE_REGISTER_ERRNO(flare::rpc::ECLOSE, "Close socket initiatively");
FLARE_REGISTER_ERRNO(flare::rpc::EITP, "Bad Itp response");


DECLARE_bool(flare_log_as_json);

namespace flare::rpc {

    DEFINE_bool(graceful_quit_on_sigterm, false,
                "Register SIGTERM handle func to quit graceful");

    const IdlNames idl_single_req_single_res = {"req", "res"};
    const IdlNames idl_single_req_multi_res = {"req", ""};
    const IdlNames idl_multi_req_single_res = {"", "res"};
    const IdlNames idl_multi_req_multi_res = {"", ""};

    extern const int64_t IDL_VOID_RESULT = 12345678987654321LL;

// For definitely false branch in flare/rpc/profiler_link.h
    int PROFILER_LINKER_DUMMY = 0;

    static void PrintRevision(std::ostream &os, void *) {
#if defined(FLARE_RPC_REVISION)
        os << FLARE_RPC_REVISION;
#else
        os << "undefined";
#endif
    }

    static flare::status_gauge<std::string> s_rpc_revision(
            "rpc_revision", PrintRevision, nullptr);

    static const int RETRY_AVOIDANCE = 8;

    // Defined in parallel_channel.cpp
    void DestroyParallelChannelDone(google::protobuf::Closure *c);

    const Controller *GetSubControllerOfParallelChannel(
            const google::protobuf::Closure *done, int index);

    const Controller *GetSubControllerOfSelectiveChannel(
            const RPCSender *sender, int index);

    DECLARE_bool(usercode_in_pthread);
    static const int MAX_RETRY_COUNT = 1000;
    static flare::gauge<int64_t> *g_ncontroller = nullptr;

    static pthread_once_t s_create_vars_once = PTHREAD_ONCE_INIT;

    static void CreateVars() {
        g_ncontroller = new flare::gauge<int64_t>("rpc_controller_count");
    }

    Controller::Controller() {
        FLARE_CHECK_EQ(0, pthread_once(&s_create_vars_once, CreateVars));
        *g_ncontroller << 1;
        ResetPods();
    }

    Controller::Controller(const Inheritable &parent_ctx) {
        FLARE_CHECK_EQ(0, pthread_once(&s_create_vars_once, CreateVars));
        *g_ncontroller << 1;
        ResetPods();
        _inheritable = parent_ctx;
    }

    struct SessionKVFlusher {
        Controller *cntl;
    };

    static std::ostream &operator<<(std::ostream &os, const SessionKVFlusher &f) {
        f.cntl->FlushSessionKV(os);
        return os;
    }

    Controller::~Controller() {
        *g_ncontroller << -1;
        if (_session_kv != nullptr && _session_kv->Count() != 0) {
            FLARE_LOG(INFO) << SessionKVFlusher{this};
        }
        ResetNonPods();
    }

    class IgnoreAllRead : public ProgressiveReader {
    public:
        // @ProgressiveReader
        flare::result_status OnReadOnePart(const void * /*data*/, size_t /*length*/) {
            return flare::result_status::success();
        }

        void OnEndOfMessage(const flare::result_status &) {}
    };

    static IgnoreAllRead *s_ignore_all_read = nullptr;
    static pthread_once_t s_ignore_all_read_once = PTHREAD_ONCE_INIT;

    static void CreateIgnoreAllRead() { s_ignore_all_read = new IgnoreAllRead; }

    // If resource needs to be destroyed or memory needs to be deleted (both
    // directly and indirectly referenced), do them in this method. Notice that
    // you don't have to set the fields to initial state after deletion since
    // they'll be set uniformly after this method is called.
    void Controller::ResetNonPods() {
        if (_span) {
            Span::Submit(_span, flare::get_current_time_micros());
        }
        _error_text.clear();
        _remote_side = flare::base::end_point();
        _local_side = flare::base::end_point();
        if (_session_local_data) {
            _server->_session_local_data_pool->Return(_session_local_data);
        }
        _mongo_session_data.reset();
        delete _sampled_request;

        if (!is_used_by_rpc() && _correlation_id != INVALID_FIBER_TOKEN) {
            FLARE_CHECK_NE(EPERM, fiber_token_cancel(_correlation_id));
        }
        if (_oncancel_id != INVALID_FIBER_TOKEN) {
            fiber_token_error(_oncancel_id, 0);
        }
        if (_pchan_sub_count > 0) {
            DestroyParallelChannelDone(_done);
        }
        delete _sender;
        _lb.reset(nullptr);
        _current_call.Reset();
        ExcludedServers::Destroy(_accessed);
        _request_buf.clear();
        delete _http_request;
        delete _http_response;
        _request_attachment.clear();
        _response_attachment.clear();
        if (_wpa) {
            _wpa->MarkRPCAsDone(Failed());
            _wpa.reset(nullptr);
        }
        if (_rpa != nullptr) {
            if (!has_progressive_reader()) {
                // Never called ReadProgressiveAttachmentBy (successfully), the data
                // is probably being buffered and a full buffer may block parse
                // handler of the protocol. We need to set a reader to consume
                // the buffer.
                pthread_once(&s_ignore_all_read_once, CreateIgnoreAllRead);
                _rpa->ReadProgressiveAttachmentBy(s_ignore_all_read);
            }
            _rpa.reset(nullptr);
        }
        delete _remote_stream_settings;
        _thrift_method_name.clear();

        FLARE_CHECK(_unfinished_call == nullptr);
    }

    void Controller::ResetPods() {
        // NOTE: Make the sequence of assignments same with the order that they're
        // defined in header. Better for cpu cache and faster for lookup.
        _span = nullptr;
        _flags = 0;
        set_pb_bytes_to_base64(true);
        _error_code = 0;
        _session_local_data = nullptr;
        _server = nullptr;
        _oncancel_id = INVALID_FIBER_TOKEN;
        _auth_context = nullptr;
        _sampled_request = nullptr;
        _request_protocol = PROTOCOL_UNKNOWN;
        _max_retry = UNSET_MAGIC_NUM;
        _retry_policy = nullptr;
        _correlation_id = INVALID_FIBER_TOKEN;
        _connection_type = CONNECTION_TYPE_UNKNOWN;
        _timeout_ms = UNSET_MAGIC_NUM;
        _backup_request_ms = UNSET_MAGIC_NUM;
        _connect_timeout_ms = UNSET_MAGIC_NUM;
        _deadline_us = -1;
        _timeout_id = 0;
        _begin_time_us = 0;
        _end_time_us = 0;
        _tos = 0;
        _preferred_index = -1;
        _request_compress_type = COMPRESS_TYPE_NONE;
        _response_compress_type = COMPRESS_TYPE_NONE;
        _fail_limit = UNSET_MAGIC_NUM;
        _pipelined_count = 0;
        _inheritable.Reset();
        _pchan_sub_count = 0;
        _response = nullptr;
        _done = nullptr;
        _sender = nullptr;
        _request_code = 0;
        _single_server_id = INVALID_SOCKET_ID;
        _unfinished_call = nullptr;
        _stream_creator = nullptr;
        _accessed = nullptr;
        _pack_request = nullptr;
        _method = nullptr;
        _auth = nullptr;
        _idl_names = idl_single_req_single_res;
        _idl_result = IDL_VOID_RESULT;
        _http_request = nullptr;
        _http_response = nullptr;
        _request_stream = INVALID_STREAM_ID;
        _response_stream = INVALID_STREAM_ID;
        _remote_stream_settings = nullptr;
    }

    Controller::Call::Call(Controller::Call *rhs)
            : nretry(rhs->nretry), need_feedback(rhs->need_feedback),
              enable_circuit_breaker(rhs->enable_circuit_breaker), peer_id(rhs->peer_id),
              begin_time_us(rhs->begin_time_us), sending_sock(rhs->sending_sock.release()),
              stream_user_data(rhs->stream_user_data) {
        // NOTE: fields in rhs should be reset because RPC could fail before
        // setting all the fields to next call and _current_call.OnComplete
        // will behave incorrectly.
        rhs->need_feedback = false;
        rhs->peer_id = INVALID_SOCKET_ID;
        rhs->stream_user_data = nullptr;
    }

    Controller::Call::~Call() {
        FLARE_CHECK(sending_sock.get() == nullptr);
    }

    void Controller::Call::Reset() {
        nretry = 0;
        need_feedback = false;
        enable_circuit_breaker = false;
        peer_id = INVALID_SOCKET_ID;
        begin_time_us = 0;
        sending_sock.reset(nullptr);
        stream_user_data = nullptr;
    }

    void Controller::set_timeout_ms(int64_t timeout_ms) {
        if (timeout_ms <= 0x7fffffff) {
            _timeout_ms = timeout_ms;
        } else {
            _timeout_ms = 0x7fffffff;
            FLARE_LOG(WARNING) << "timeout_ms is limited to 0x7fffffff (roughly 24 days)";
        }
    }

    void Controller::set_backup_request_ms(int64_t timeout_ms) {
        if (timeout_ms <= 0x7fffffff) {
            _backup_request_ms = timeout_ms;
        } else {
            _backup_request_ms = 0x7fffffff;
            FLARE_LOG(WARNING) << "backup_request_ms is limited to 0x7fffffff (roughly 24 days)";
        }
    }

    void Controller::set_max_retry(int max_retry) {
        if (max_retry > MAX_RETRY_COUNT) {
            FLARE_LOG(WARNING) << "Retry count can't be larger than "
                               << MAX_RETRY_COUNT << ", round it to "
                               << MAX_RETRY_COUNT;
            _max_retry = MAX_RETRY_COUNT;
        } else {
            _max_retry = max_retry;
        }
    }

    void Controller::set_log_id(uint64_t log_id) {
        add_flag(FLAGS_LOG_ID);
        _inheritable.log_id = log_id;
    }


    bool Controller::Failed() const {
        return FailedInline();
    }

    std::string Controller::ErrorText() const {
        return _error_text;
    }

    void StartCancel(CallId id) {
        fiber_token_error(id, ECANCELED);
    }

    void Controller::StartCancel() {
        FLARE_LOG(FATAL) << "You must call flare::rpc::StartCancel(id) instead!"
                            " because this function is racing with ~Controller() in "
                            " asynchronous calls.";
    }

    static const char HEX_ALPHA[] = "0123456789ABCDEF";

    void Controller::AppendServerIdentiy() {
        if (_server == nullptr) {
            return;
        }
        if (is_security_mode()) {
            _error_text.reserve(_error_text.size() + MD5_DIGEST_LENGTH * 2 + 2);
            _error_text.push_back('[');
            char ipbuf[64];
            int len = snprintf(ipbuf, sizeof(ipbuf), "%s:%d",
                               flare::base::my_ip_cstr(), _server->listen_address().port);
            unsigned char digest[MD5_DIGEST_LENGTH];
            MD5((const unsigned char *) ipbuf, len, digest);
            for (size_t i = 0; i < sizeof(digest); ++i) {
                _error_text.push_back(HEX_ALPHA[digest[i] & 0xF]);
                _error_text.push_back(HEX_ALPHA[digest[i] >> 4]);
            }
            _error_text.push_back(']');
        } else {
            flare::string_appendf(&_error_text, "[%s:%d]",
                                  flare::base::my_ip_cstr(), _server->listen_address().port);
        }
    }

    inline void UpdateResponseHeader(Controller *cntl) {
        FLARE_DCHECK(cntl->Failed());
        if (cntl->request_protocol() == PROTOCOL_HTTP ||
            cntl->request_protocol() == PROTOCOL_H2) {
            if (cntl->ErrorCode() != EHTTP) {
                // Set the related status code
                cntl->http_response().set_status_code(
                        ErrorCodeToStatusCode(cntl->ErrorCode()));
            } // else assume that status code is already set along with EHTTP.
            if (cntl->server() != nullptr) {
                // Override HTTP body at server-side to conduct error text
                // to the client.
                // The client-side should preserve body which may be a piece
                // of useable data rather than error text.
                cntl->response_attachment().clear();
                cntl->response_attachment().append(cntl->ErrorText());
            }
        }
    }

    void Controller::SetFailed(const std::string &reason) {
        _error_code = -1;
        if (!_error_text.empty()) {
            _error_text.push_back(' ');
        }
        if (_current_call.nretry != 0) {
            flare::string_appendf(&_error_text, "[R%d]", _current_call.nretry);
        } else {
            AppendServerIdentiy();
        }
        _error_text.append(reason);
        if (_span) {
            _span->set_error_code(_error_code);
            _span->Annotate(reason);
        }
        UpdateResponseHeader(this);
    }

    void Controller::SetFailed(int error_code, const char *reason_fmt, ...) {
        if (error_code == 0) {
            FLARE_CHECK(false) << "error_code is 0";
            error_code = -1;
        }
        _error_code = error_code;
        if (!_error_text.empty()) {
            _error_text.push_back(' ');
        }
        if (_current_call.nretry != 0) {
            flare::string_appendf(&_error_text, "[R%d]", _current_call.nretry);
        } else {
            AppendServerIdentiy();
        }
        const size_t old_size = _error_text.size();
        if (_error_code != -1) {
            flare::string_appendf(&_error_text, "[E%d]", _error_code);
        }
        va_list ap;
        va_start(ap, reason_fmt);
        flare::string_vappendf(&_error_text, reason_fmt, ap);
        va_end(ap);
        if (_span) {
            _span->set_error_code(_error_code);
            _span->AnnotateCStr(_error_text.c_str() + old_size, 0);
        }
        UpdateResponseHeader(this);
    }

    void Controller::CloseConnection(const char *reason_fmt, ...) {
        if (_error_code == 0) {
            _error_code = ECLOSE;
        }
        add_flag(FLAGS_CLOSE_CONNECTION);
        if (!_error_text.empty()) {
            _error_text.push_back(' ');
        }
        if (_current_call.nretry != 0) {
            flare::string_appendf(&_error_text, "[R%d]", _current_call.nretry);
        } else {
            AppendServerIdentiy();
        }
        const size_t old_size = _error_text.size();
        if (_error_code != -1) {
            flare::string_appendf(&_error_text, "[E%d]", _error_code);
        }
        va_list ap;
        va_start(ap, reason_fmt);
        flare::string_vappendf(&_error_text, reason_fmt, ap);
        va_end(ap);
        if (_span) {
            _span->set_error_code(_error_code);
            _span->AnnotateCStr(_error_text.c_str() + old_size, 0);
        }
        UpdateResponseHeader(this);
    }

    bool Controller::IsCanceled() const {
        SocketUniquePtr sock;
        return (Socket::Address(_current_call.peer_id, &sock) != 0);
    }

    class RunOnCancelThread {
    public:
        RunOnCancelThread(google::protobuf::Closure *cb, fiber_token_t id)
                : _cb(cb), _id(id) {}

        static void *RunThis(void *arg) {
            ((RunOnCancelThread *) arg)->Run();
            return nullptr;
        }

        void Run() {
            _cb->Run();
            FLARE_CHECK_EQ(0, fiber_token_unlock_and_destroy(_id));
            delete this;
        }

    private:
        google::protobuf::Closure *_cb;
        fiber_token_t _id;
    };

    int Controller::RunOnCancel(fiber_token_t id, void *data, int error_code) {
        if (error_code == 0) {
            // Called from Controller::ResetNonPods upon Controller's Reset or
            // destruction, we just call the callback in-place.
            static_cast<google::protobuf::Closure *>(data)->Run();
            FLARE_CHECK_EQ(0, fiber_token_unlock_and_destroy(id));
            return 0;
        }
        // Called from Socket::SetFailed, should be infrequent.
        // To make sure Socket::SetFailed is never blocked, we run the callback
        // in a new thread.
        RunOnCancelThread *arg = new RunOnCancelThread(
                static_cast<google::protobuf::Closure *>(data), id);
        fiber_id_t th;
        FLARE_CHECK_EQ(0, fiber_start_urgent(&th, nullptr, RunOnCancelThread::RunThis, arg));
        return 0;
    }

    void Controller::NotifyOnCancel(google::protobuf::Closure *callback) {
        if (nullptr == callback) {
            FLARE_LOG(WARNING) << "Parameter `callback' is NLLL";
            return;
        }

        ClosureGuard guard(callback);
        if (_oncancel_id != INVALID_FIBER_TOKEN) {
            FLARE_LOG(FATAL) << "NotifyCancel a single call more than once!";
            return;
        }
        if (fiber_token_create(&_oncancel_id, callback, RunOnCancel) != 0) {
            FLARE_PLOG(FATAL) << "Fail to create fiber_token";
            return;
        }
        SocketUniquePtr sock;
        if (Socket::Address(_current_call.peer_id, &sock) != 0) {
            // Connection already broken
            return;
        }
        sock->NotifyOnFailed(_oncancel_id);  // Always succeed
        guard.release();
    }

    void Join(CallId id) {
        fiber_token_join(id);
    }

    void JoinResponse(CallId id) {
        fiber_token_join(id);
    }

    static void HandleTimeout(void *arg) {
        fiber_token_t correlation_id = {(uint64_t) arg};
        fiber_token_error(correlation_id, ERPCTIMEDOUT);
    }

    void Controller::OnVersionedRPCReturned(const CompletionInfo &info,
                                            bool new_fiber, int saved_error) {
        // TODO(gejun): Simplify call-ending code.
        // Intercept previous calls
        while (info.id != _correlation_id && info.id != current_id()) {
            if (_unfinished_call && get_id(_unfinished_call->nretry) == info.id) {
                if (!FailedInline()) {
                    // Continue with successful backup request.
                    break;
                }
                // Complete failed backup request.
                _unfinished_call->OnComplete(this, _error_code, info.responded, false);
                delete _unfinished_call;
                _unfinished_call = nullptr;
            }
            // Ignore all non-backup requests and failed backup requests.
            _error_code = saved_error;
            response_attachment().clear();
            FLARE_CHECK_EQ(0, fiber_token_unlock(info.id));
            return;
        }

        if ((!_error_code && _retry_policy == nullptr) ||
            _current_call.nretry >= _max_retry) {
            goto END_OF_RPC;
        }
        if (_error_code == EBACKUPREQUEST) {
            // Reset timeout if needed
            int rc = 0;
            if (timeout_ms() >= 0) {
                rc = fiber_timer_add(
                        &_timeout_id,
                        flare::time_point::from_unix_micros(_deadline_us).to_timespec(),
                        HandleTimeout, (void *) _correlation_id.value);
            }
            if (rc != 0) {
                SetFailed(rc, "Fail to add timer");
                goto END_OF_RPC;
            }
            if (!SingleServer()) {
                if (_accessed == nullptr) {
                    _accessed = ExcludedServers::Create(
                            std::min(_max_retry, RETRY_AVOIDANCE));
                    if (nullptr == _accessed) {
                        SetFailed(ENOMEM, "Fail to create ExcludedServers");
                        goto END_OF_RPC;
                    }
                }
                _accessed->Add(_current_call.peer_id);
            }
            // _current_call does not end yet.
            FLARE_CHECK(_unfinished_call == nullptr);  // only one backup request now.
            _unfinished_call = new(std::nothrow) Call(&_current_call);
            if (_unfinished_call == nullptr) {
                SetFailed(ENOMEM, "Fail to new Call");
                goto END_OF_RPC;
            }
            ++_current_call.nretry;
            add_flag(FLAGS_BACKUP_REQUEST);
            return IssueRPC(flare::get_current_time_micros());
        } else if (_retry_policy ? _retry_policy->DoRetry(this)
                                 : DefaultRetryPolicy()->DoRetry(this)) {
            // The error must come from _current_call because:
            //  * we intercepted error from _unfinished_call in OnVersionedRPCReturned
            //  * ERPCTIMEDOUT/ECANCELED are not retrying error by default.
            FLARE_CHECK_EQ(current_id(), info.id) << "error_code=" << _error_code;
            if (!SingleServer()) {
                if (_accessed == nullptr) {
                    _accessed = ExcludedServers::Create(
                            std::min(_max_retry, RETRY_AVOIDANCE));
                    if (nullptr == _accessed) {
                        SetFailed(ENOMEM, "Fail to create ExcludedServers");
                        goto END_OF_RPC;
                    }
                }
                _accessed->Add(_current_call.peer_id);
            }
            _current_call.OnComplete(this, _error_code, info.responded, false);
            ++_current_call.nretry;
            // Clear http responses before retrying, otherwise the response may
            // be mixed with older (and undefined) stuff. This is actually not
            // done before r32008.
            if (_http_response) {
                _http_response->Clear();
            }
            response_attachment().clear();
            return IssueRPC(flare::get_current_time_micros());
        }

        END_OF_RPC:
        if (new_fiber) {
            // [ Essential for -usercode_in_pthread=true ]
            // When -usercode_in_pthread is on, the reserved threads (set by
            // -usercode_backup_threads) may all block on fiber_token_lock in
            // ProcessXXXResponse(), until the id is unlocked or destroyed which
            // is run in a new thread when new_fiber is true. However since all
            // workers are blocked, the created fiber will never be scheduled
            // and result in deadlock.
            // Make the id unlockable before creating the fiber fixes the issue.
            // When -usercode_in_pthread is false, this also removes some useless
            // waiting of the fibers processing responses.

            // Note[_done]: callid is destroyed after _done which possibly takes
            // a lot of time, stop useless locking

            // Note[cid]: When the callid needs to be destroyed in done->Run(),
            // it does not mean that it will be destroyed directly in done->Run(),
            // conversely the callid may still be locked/unlocked for many times
            // before destroying. E.g. in slective channel, the callid is referenced
            // by multiple sub-done and only destroyed by the last one. Calling
            // fiber_token_about_to_destroy right here which makes the id unlockable
            // anymore, is wrong. On the other hand, the combo channles setting
            // FLAGS_DESTROY_CID_IN_DONE to true must be aware of
            // -usercode_in_pthread and avoid deadlock by their own (TBR)

            if ((FLAGS_usercode_in_pthread || _done != nullptr/*Note[_done]*/) &&
                !has_flag(FLAGS_DESTROY_CID_IN_DONE)/*Note[cid]*/) {
                fiber_token_about_to_destroy(info.id);
            }
            // No need to join this fiber since RPC caller won't wake up
            // (or user's done won't be called) until this fiber finishes
            fiber_id_t bt;
            fiber_attribute attr = (FLAGS_usercode_in_pthread ?
                                    FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL);
            _tmp_completion_info = info;
            if (fiber_start_background(&bt, &attr, RunEndRPC, this) != 0) {
                FLARE_LOG(FATAL) << "Fail to start fiber";
                EndRPC(info);
            }
        } else {
            if (_done != nullptr/*Note[_done]*/ &&
                !has_flag(FLAGS_DESTROY_CID_IN_DONE)/*Note[cid]*/) {
                fiber_token_about_to_destroy(info.id);
            }
            EndRPC(info);
        }
    }

    void *Controller::RunEndRPC(void *arg) {
        Controller *c = static_cast<Controller *>(arg);
        c->EndRPC(c->_tmp_completion_info);
        return nullptr;
    }

    inline bool does_error_affect_main_socket(int error_code) {
        // Errors tested in this function are reported by pooled connections
        // and very likely to indicate that the server-side is down and the socket
        // should be health-checked.
        return error_code == ECONNREFUSED ||
               error_code == ENETUNREACH ||
               error_code == EHOSTUNREACH ||
               error_code == EINVAL/*returned by connect "0.0.0.1"*/;
    }

//Note: A RPC call is probably consisted by several individual Calls such as
//      retries and backup requests. This method simply cares about the error of
//      this very Call (specified by |error_code|) rather than the error of the
//      entire RPC (specified by c->FailedInline()).
    void Controller::Call::OnComplete(
            Controller *c, int error_code/*note*/, bool responded, bool end_of_rpc) {
        if (stream_user_data) {
            stream_user_data->DestroyStreamUserData(sending_sock, c, error_code, end_of_rpc);
            stream_user_data = nullptr;
        }

        if (sending_sock != nullptr) {
            if (error_code != 0) {
                sending_sock->AddRecentError();
            }

            if (enable_circuit_breaker) {
                sending_sock->FeedbackCircuitBreaker(error_code,
                                                     flare::get_current_time_micros() - begin_time_us);
            }
        }

        switch (c->connection_type()) {
            case CONNECTION_TYPE_UNKNOWN:
                break;
            case CONNECTION_TYPE_SINGLE:
                // Set main socket to be failed for connection refusal of streams.
                // "single" streams are often maintained in a separate SocketMap and
                // different from the main socket as well.
                if (c->_stream_creator != nullptr &&
                    does_error_affect_main_socket(error_code) &&
                    (sending_sock == nullptr || sending_sock->id() != peer_id)) {
                    Socket::SetFailed(peer_id);
                }
                break;
            case CONNECTION_TYPE_POOLED:
                // NOTE: Not reuse pooled connection if this call fails and no response
                // has been received through this connection
                // Otherwise in-flight responses may come back in future and break the
                // assumption that one pooled connection cannot have more than one
                // message at the same time.
                if (sending_sock != nullptr && (error_code == 0 || responded)) {
                    if (!sending_sock->is_read_progressive()) {
                        // Normally-read socket which will not be used after RPC ends,
                        // safe to return. Notice that Socket::is_read_progressive may
                        // differ from Controller::is_response_read_progressively()
                        // because RPC possibly ends before setting up the socket.
                        sending_sock->ReturnToPool();
                    } else {
                        // Progressively-read socket. Should be returned when the read
                        // ends. The method handles the details.
                        sending_sock->OnProgressiveReadCompleted();
                    }
                    break;
                }
                // fall through
            case CONNECTION_TYPE_SHORT:
                if (sending_sock != nullptr) {
                    // Check the comment in CONNECTION_TYPE_POOLED branch.
                    if (!sending_sock->is_read_progressive()) {
                        if (c->_stream_creator == nullptr) {
                            sending_sock->SetFailed();
                        }
                    } else {
                        sending_sock->OnProgressiveReadCompleted();
                    }
                }
                if (does_error_affect_main_socket(error_code)) {
                    // main socket should die as well.
                    // NOTE: main socket may be wrongly set failed (provided that
                    // short/pooled socket does not hold a ref of the main socket).
                    // E.g. an in-parallel RPC sets the peer_id to be failed
                    //   -> this RPC meets ECONNREFUSED
                    //   -> main socket gets revived from HC
                    //   -> this RPC sets main socket to be failed again.
                    Socket::SetFailed(peer_id);
                }
                break;
        }

        if (ELOGOFF == error_code) {
            SocketUniquePtr sock;
            if (Socket::Address(peer_id, &sock) == 0) {
                // Block this `Socket' while not closing the fd
                sock->SetLogOff();
            }
        }

        if (need_feedback) {
            const LoadBalancer::CallInfo info =
                    {begin_time_us, peer_id, error_code, c};
            c->_lb->Feedback(info);
        }

        // Release the `Socket' we used to send/receive data
        sending_sock.reset(nullptr);
    }

    void Controller::EndRPC(const CompletionInfo &info) {
        if (_timeout_id != 0) {
            fiber_timer_del(_timeout_id);
            _timeout_id = 0;
        }

        // End _current_call and _unfinished_call.
        if (info.id == current_id() || info.id == _correlation_id) {
            if (_current_call.sending_sock != nullptr) {
                _remote_side = _current_call.sending_sock->remote_side();
                _local_side = _current_call.sending_sock->local_side();
            }

            if (_unfinished_call != nullptr) {
                // When _current_call is successful, mark _unfinished_call as
                // EBACKUPREQUEST, we can't use 0 because the server possibly
                // never respond, we can't use ERPCTIMEDOUT because _current_call
                // is sent after _unfinished_call which is not necessarily timedout
                // When _current_call is error, mark _unfinished_call with the
                // same error. This is not accurate as well, but we have to end
                // _unfinished_call with some sort of error anyway.
                const int err = (_error_code == 0 ? EBACKUPREQUEST : _error_code);
                _unfinished_call->OnComplete(this, err, false, false);
                delete _unfinished_call;
                _unfinished_call = nullptr;
            }
            // TODO: Replace this with stream_creator.
            HandleStreamConnection(_current_call.sending_sock.get());
            _current_call.OnComplete(this, _error_code, info.responded, true);
        } else {
            // Even if _unfinished_call succeeded, we don't use EBACKUPREQUEST
            // (which gets punished in LALB) for _current_call because _current_call
            // is sent after _unfinished_call, it's just normal that _current_call
            // does not respond before _unfinished_call.
            if (_unfinished_call == nullptr) {
                FLARE_CHECK(false) << "A previous non-backup request responded, cid="
                                   << info.id << " current_cid=" << current_id()
                                   << " initial_cid=" << _correlation_id
                                   << " stream_user_data=" << _current_call.stream_user_data
                                   << " sending_sock=" << _current_call.sending_sock.get();
            }
            _current_call.OnComplete(this, ECANCELED, false, false);
            if (_unfinished_call != nullptr) {
                if (_unfinished_call->sending_sock != nullptr) {
                    _remote_side = _unfinished_call->sending_sock->remote_side();
                    _local_side = _unfinished_call->sending_sock->local_side();
                }
                // TODO: Replace this with stream_creator.
                HandleStreamConnection(_unfinished_call->sending_sock.get());
                if (get_id(_unfinished_call->nretry) == info.id) {
                    _unfinished_call->OnComplete(
                            this, _error_code, info.responded, true);
                } else {
                    FLARE_CHECK(false) << "A previous non-backup request responded";
                    _unfinished_call->OnComplete(this, ECANCELED, false, true);
                }

                delete _unfinished_call;
                _unfinished_call = nullptr;
            }
        }
        if (_stream_creator) {
            _stream_creator->DestroyStreamCreator(this);
            _stream_creator = nullptr;
        }
        // Clear _error_text when the call succeeded, otherwise a successful
        // call with non-empty ErrorText may confuse user.
        if (!_error_code) {
            _error_text.clear();
        }
        // RPC finished, now it's safe to release `LoadBalancerWithNaming'
        _lb.reset();
        if (_span) {
            _span->set_ending_cid(info.id);
            _span->set_async(_done);
            // Submit the span if we're in async RPC. For sync RPC, the span
            // is submitted after Join() to get a more accurate resuming timestamp.
            if (_done) {
                SubmitSpan();
            }
        }

        // No need to retry or can't retry, just call user's `done'.
        const CallId saved_cid = _correlation_id;
        if (_done) {
            if (!FLAGS_usercode_in_pthread || _done == DoNothing()/*Note*/) {
                // Note: no need to run DoNothing in backup thread when pthread
                // mode is on. Otherwise there's a tricky deadlock:
                // void SomeService::CallMethod(...) { // -usercode_in_pthread=true
                //   ...
                //   channel.CallMethod(...., flare::rpc::DoNothing());
                //   flare::rpc::Join(cntl.call_id());
                //   ...
                // }
                // Join is not signalled when the done does not Run() and the done
                // can't Run() because all backup threads are blocked by Join().

                OnRPCEnd(flare::get_current_time_micros());
                const bool destroy_cid_in_done = has_flag(FLAGS_DESTROY_CID_IN_DONE);
                _done->Run();
                // NOTE: Don't touch this Controller anymore, because it's likely to be
                // deleted by done.
                if (!destroy_cid_in_done) {
                    // Make this thread not scheduling itself when launching new
                    // fibers, saving signalings.
                    // FIXME: We're assuming the calling thread is about to quit.
                    fiber_about_to_quit();
                    FLARE_CHECK_EQ(0, fiber_token_unlock_and_destroy(saved_cid));
                }
            } else {
                RunUserCode(RunDoneInBackupThread, this);
            }
        } else {
            // OnRPCEnd for sync RPC is called in Channel::CallMethod to count in
            // latency of the context-switch.

            // Check comments in above branch on fiber_about_to_quit.
            fiber_about_to_quit();
            FLARE_CHECK_EQ(0, fiber_token_unlock_and_destroy(saved_cid));
        }
    }

    void Controller::RunDoneInBackupThread(void *arg) {
        static_cast<Controller *>(arg)->DoneInBackupThread();
    }

    void Controller::DoneInBackupThread() {
        // OnRPCEnd for sync RPC is called in Channel::CallMethod to count in
        // latency of the context-switch.
        OnRPCEnd(flare::get_current_time_micros());
        const CallId saved_cid = _correlation_id;
        const bool destroy_cid_in_done = has_flag(FLAGS_DESTROY_CID_IN_DONE);
        _done->Run();
        // NOTE: Don't touch fields of controller anymore, it may be deleted.
        if (!destroy_cid_in_done) {
            FLARE_CHECK_EQ(0, fiber_token_unlock_and_destroy(saved_cid));
        }
    }

    void Controller::SubmitSpan() {
        const int64_t now = flare::get_current_time_micros();
        _span->set_start_callback_us(now);
        if (_span->local_parent()) {
            _span->local_parent()->AsParent();
        }
        Span::Submit(_span, now);
        _span = nullptr;
    }

    void Controller::HandleSendFailed() {
        if (!FailedInline()) {
            SetFailed("Must be SetFailed() before calling HandleSendFailed()");
            FLARE_LOG(FATAL) << ErrorText();
        }
        const CompletionInfo info = {current_id(), false};
        // NOTE: Launch new thread to run the callback in an asynchronous call
        // (and done is not allowed to run in-place)
        // Users may hold a lock before asynchronus CallMethod returns and
        // grab the same lock inside done->Run(). If done->Run() is called in the
        // same stack of CallMethod, the code is deadlocked.
        // We don't need to run the callback in new thread in a sync call since
        // the created thread needs to be joined anyway before end of CallMethod.
        const bool new_fiber = (_done != nullptr && !is_done_allowed_to_run_in_place());
        OnVersionedRPCReturned(info, new_fiber, _error_code);
    }

    void Controller::IssueRPC(int64_t start_realtime_us) {
        _current_call.begin_time_us = start_realtime_us;
        // Clear last error, Don't clear _error_text because we append to it.
        _error_code = 0;

        // Make versioned correlation_id.
        // call_id         : unversioned, mainly for ECANCELED and ERPCTIMEDOUT
        // call_id + 1     : first try.
        // call_id + 2     : retry 1
        // ...
        // call_id + N + 1 : retry N
        // All ids except call_id are versioned. Say if we've sent retry 1 and
        // a failed response of first try comes back, it will be ignored.
        const CallId cid = current_id();

        // Intercept IssueRPC when _sender is set. Currently _sender is only set
        // by SelectiveChannel.
        if (_sender) {
            if (_sender->IssueRPC(start_realtime_us) != 0) {
                return HandleSendFailed();
            }
            FLARE_CHECK_EQ(0, fiber_token_unlock(cid));
            return;
        }

        // Pick a target server for sending RPC
        _current_call.need_feedback = false;
        _current_call.enable_circuit_breaker = has_enabled_circuit_breaker();
        SocketUniquePtr tmp_sock;
        if (SingleServer()) {
            // Don't use _current_call.peer_id which is set to -1 after construction
            // of the backup call.
            const int rc = Socket::Address(_single_server_id, &tmp_sock);
            if (rc != 0 || (!is_health_check_call() && !tmp_sock->IsAvailable())) {
                SetFailed(EHOSTDOWN, "Not connected to %s yet, server_id=%" PRIu64,
                          endpoint2str(_remote_side).c_str(), _single_server_id);
                tmp_sock.reset();  // Release ref ASAP
                return HandleSendFailed();
            }
            _current_call.peer_id = _single_server_id;
        } else {
            LoadBalancer::SelectIn sel_in =
                    {start_realtime_us, true,
                     has_request_code(), _request_code, _accessed};
            LoadBalancer::SelectOut sel_out(&tmp_sock);
            const int rc = _lb->SelectServer(sel_in, &sel_out);
            if (rc != 0) {
                std::ostringstream os;
                DescribeOptions opt;
                opt.verbose = false;
                _lb->Describe(os, opt);
                SetFailed(rc, "Fail to select server from %s", os.str().c_str());
                return HandleSendFailed();
            }
            _current_call.need_feedback = sel_out.need_feedback;
            _current_call.peer_id = tmp_sock->id();
            // NOTE: _remote_side must be set here because _pack_request below
            // may need it (e.g. http may set "Host" to _remote_side)
            // Don't set _local_side here because tmp_sock may be not connected
            // here.
            _remote_side = tmp_sock->remote_side();
        }
        if (_stream_creator) {
            _current_call.stream_user_data =
                    _stream_creator->OnCreatingStream(&tmp_sock, this);
            if (FailedInline()) {
                return HandleSendFailed();
            }
            // remote_side can't be changed.
            FLARE_CHECK_EQ(_remote_side, tmp_sock->remote_side());
        }

        Span *span = _span;
        if (span) {
            if (_current_call.nretry == 0) {
                span->set_remote_side(_remote_side);
            } else {
                span->Annotate("Retrying %s",
                               endpoint2str(_remote_side).c_str());
            }
        }
        // Handle connection type
        if (_connection_type == CONNECTION_TYPE_SINGLE ||
            _stream_creator != nullptr) { // let user decides the sending_sock
            // in the callback(according to connection_type) directly
            _current_call.sending_sock.reset(tmp_sock.release());
            // TODO(gejun): Setting preferred index of single-connected socket
            // has two issues:
            //   1. race conditions. If a set perferred_index is overwritten by
            //      another thread, the response back has to check protocols one
            //      by one. This is a performance issue, correctness is unaffected.
            //   2. thrashing between different protocols. Also a performance issue.
            _current_call.sending_sock->set_preferred_index(_preferred_index);
        } else {
            int rc = 0;
            if (_connection_type == CONNECTION_TYPE_POOLED) {
                rc = tmp_sock->GetPooledSocket(&_current_call.sending_sock);
            } else if (_connection_type == CONNECTION_TYPE_SHORT) {
                rc = tmp_sock->GetShortSocket(&_current_call.sending_sock);
            } else {
                tmp_sock.reset();
                SetFailed(EINVAL, "Invalid connection_type=%d", (int) _connection_type);
                return HandleSendFailed();
            }
            if (rc) {
                tmp_sock.reset();
                SetFailed(rc, "Fail to get %s connection",
                          ConnectionTypeToString(_connection_type));
                return HandleSendFailed();
            }
            // Remember the preferred protocol for non-single connection. When
            // the response comes back, InputMessenger calls the right handler
            // w/o trying other protocols. This is a must for (many) protocols that
            // can't be distinguished from other protocols w/o ambiguity.
            _current_call.sending_sock->set_preferred_index(_preferred_index);
            // Set preferred_index of main_socket as well to make it easier to
            // debug and observe from /connections.
            if (tmp_sock->preferred_index() < 0) {
                tmp_sock->set_preferred_index(_preferred_index);
            }
            tmp_sock.reset();
        }
        if (_tos > 0) {
            _current_call.sending_sock->set_type_of_service(_tos);
        }
        if (is_response_read_progressively()) {
            // Tag the socket so that when the response comes back, the parser will
            // stop before reading all body.
            _current_call.sending_sock->read_will_be_progressive(_connection_type);
        }

        // Handle authentication
        const Authenticator *using_auth = nullptr;
        if (_auth != nullptr) {
            // Only one thread will be the winner and get the right to pack
            // authentication information, others wait until the request
            // is sent.
            int auth_error = 0;
            if (_current_call.sending_sock->FightAuthentication(&auth_error) == 0) {
                using_auth = _auth;
            } else if (auth_error != 0) {
                SetFailed(auth_error, "Fail to authenticate, %s",
                          flare_error(auth_error));
                return HandleSendFailed();
            }
        }
        // Make request
        flare::cord_buf packet;
        SocketMessage *user_packet = nullptr;
        _pack_request(&packet, &user_packet, cid.value, _method, this,
                      _request_buf, using_auth);
        // TODO: PackRequest may accept SocketMessagePtr<>?
        SocketMessagePtr<> user_packet_guard(user_packet);
        if (FailedInline()) {
            // controller should already be SetFailed.
            if (using_auth) {
                // Don't forget to signal waiters on authentication
                _current_call.sending_sock->SetAuthentication(ErrorCode());
            }
            return HandleSendFailed();
        }

        timespec connect_abstime;
        timespec *pabstime = nullptr;
        if (_connect_timeout_ms > 0) {
            if (_deadline_us >= 0) {
                connect_abstime = flare::time_point::from_unix_micros(
                        std::min(_connect_timeout_ms * 1000L + start_realtime_us,
                                 _deadline_us)).to_timespec();
            } else {
                connect_abstime = flare::time_point::from_unix_micros(
                        _connect_timeout_ms * 1000L + start_realtime_us).to_timespec();
            }
            pabstime = &connect_abstime;
        }
        Socket::WriteOptions wopt;
        wopt.id_wait = cid;
        wopt.abstime = pabstime;
        wopt.pipelined_count = _pipelined_count;
        wopt.with_auth = has_flag(FLAGS_REQUEST_WITH_AUTH);
        wopt.ignore_eovercrowded = has_flag(FLAGS_IGNORE_EOVERCROWDED);
        int rc;
        size_t packet_size = 0;
        if (user_packet_guard) {
            if (span) {
                packet_size = user_packet_guard->EstimatedByteSize();
            }
            rc = _current_call.sending_sock->Write(user_packet_guard, &wopt);
        } else {
            packet_size = packet.size();
            rc = _current_call.sending_sock->Write(&packet, &wopt);
        }
        if (span) {
            if (_current_call.nretry == 0) {
                span->set_sent_us(flare::get_current_time_micros());
                span->set_request_size(packet_size);
            } else {
                span->Annotate("Requested(%lld) [%d]",
                               (long long) packet_size, _current_call.nretry + 1);
            }
        }
        if (using_auth) {
            // For performance concern, we set authentication to immediately
            // after the first `Write' returns instead of waiting for server
            // to confirm the credential data
            _current_call.sending_sock->SetAuthentication(rc);
        }
        FLARE_CHECK_EQ(0, fiber_token_unlock(cid));
    }

    void Controller::set_auth_context(const AuthContext *ctx) {
        if (_auth_context != nullptr) {
            FLARE_LOG(FATAL) << "Impossible! This function is supposed to be called "
                                "only once when verification succeeds in server side";
            return;
        }
        // Ownership is belong to `Socket' instead of `Controller'
        _auth_context = ctx;
    }

    int Controller::HandleSocketFailed(fiber_token_t id, void *data, int error_code,
                                       const std::string &error_text) {
        Controller *cntl = static_cast<Controller *>(data);
        if (!cntl->is_used_by_rpc()) {
            // Cannot destroy the call_id before RPC otherwise an async RPC
            // using the controller cannot be joined and related resources may be
            // destroyed before done->Run() running in another fiber.
            // The error set will be detected in Channel::CallMethod and fail
            // the RPC.
            cntl->SetFailed(error_code, "Cancel call_id=%" PRId64
                                        " before CallMethod()", id.value);
            return fiber_token_unlock(id);
        }
        const int saved_error = cntl->ErrorCode();
        if (error_code == ERPCTIMEDOUT) {
            cntl->SetFailed(error_code, "Reached timeout=%" PRId64 "ms @%s",
                            cntl->timeout_ms(),
                            flare::base::endpoint2str(cntl->remote_side()).c_str());
        } else if (error_code == EBACKUPREQUEST) {
            cntl->SetFailed(error_code, "Reached backup timeout=%" PRId64 "ms @%s",
                            cntl->backup_request_ms(),
                            flare::base::endpoint2str(cntl->remote_side()).c_str());
        } else if (!error_text.empty()) {
            cntl->SetFailed(error_code, "%s", error_text.c_str());
        } else {
            cntl->SetFailed(error_code, "%s @%s", flare_error(error_code),
                            flare::base::endpoint2str(cntl->remote_side()).c_str());
        }
        CompletionInfo info = {id, false};
        cntl->OnVersionedRPCReturned(info, true, saved_error);
        return 0;
    }

    CallId Controller::call_id() {
        std::atomic<uint64_t> *target =
                (std::atomic<uint64_t> *) &_correlation_id.value;
        uint64_t loaded = target->load(std::memory_order_relaxed);
        if (loaded) {
            const CallId id = {loaded};
            return id;
        }
        // Optimistic locking.
        CallId cid = {0};
        // The range of this id will be reset in Channel::CallMethod
        FLARE_CHECK_EQ(0, fiber_token_create2(&cid, this, HandleSocketFailed));
        if (!target->compare_exchange_strong(loaded, cid.value,
                                             std::memory_order_relaxed)) {
            fiber_token_cancel(cid);
            cid.value = loaded;
        }
        return cid;
    }

    void Controller::SaveClientSettings(ClientSettings *s) const {
        s->timeout_ms = _timeout_ms;
        s->backup_request_ms = _backup_request_ms;
        s->max_retry = _max_retry;
        s->tos = _tos;
        s->connection_type = _connection_type;
        s->request_compress_type = _request_compress_type;
        s->log_id = log_id();
        s->has_request_code = has_request_code();
        s->request_code = _request_code;
    }

    void Controller::ApplyClientSettings(const ClientSettings &s) {
        set_timeout_ms(s.timeout_ms);
        set_backup_request_ms(s.backup_request_ms);
        set_max_retry(s.max_retry);
        set_type_of_service(s.tos);
        set_connection_type(s.connection_type);
        set_request_compress_type(s.request_compress_type);
        set_log_id(s.log_id);
        set_flag(FLAGS_REQUEST_CODE, s.has_request_code);
        _request_code = s.request_code;
    }

    int Controller::sub_count() const {
        int n = _pchan_sub_count;
        if (_sender) {
            n += !!GetSubControllerOfSelectiveChannel(_sender, 0);
        }
        return n;
    }

    const Controller *Controller::sub(int index) const {
        if (_pchan_sub_count > 0 && _done != nullptr) {
            return GetSubControllerOfParallelChannel(_done, index);
        }
        if (_sender != nullptr) {
            return GetSubControllerOfSelectiveChannel(_sender, index);
        }
        return nullptr;
    }

    uint64_t Controller::trace_id() const { return _span ? _span->trace_id() : 0; }

    uint64_t Controller::span_id() const { return _span ? _span->span_id() : 0; }

    void *Controller::session_local_data() {
        if (_session_local_data) {
            return _session_local_data;
        }
        if (_server) {
            SimpleDataPool *pool = _server->_session_local_data_pool;
            if (pool) {
                _session_local_data = pool->Borrow();
                return _session_local_data;
            }
        }
        return nullptr;
    }

    void Controller::HandleStreamConnection(Socket *host_socket) {
        if (_request_stream == INVALID_STREAM_ID) {
            FLARE_CHECK(!has_remote_stream());
            return;
        }
        SocketUniquePtr ptr;
        if (!FailedInline()) {
            if (Socket::Address(_request_stream, &ptr) != 0) {
                if (!FailedInline()) {
                    SetFailed(EREQUEST, "Request stream=%" PRIu64 " was closed before responded",
                              _request_stream);
                }
            } else if (_remote_stream_settings == nullptr) {
                if (!FailedInline()) {
                    SetFailed(EREQUEST, "The server didn't accept the stream");
                }
            }
        }
        if (FailedInline()) {
            Stream::SetFailed(_request_stream);
            if (_remote_stream_settings != nullptr) {
                policy::SendStreamRst(host_socket,
                                      _remote_stream_settings->stream_id());
            }
            return;
        }
        Stream *s = (Stream *) ptr->conn();
        s->SetConnected(_remote_stream_settings);
    }

// TODO: Need more security advices from professionals.
// TODO: Is percent encoding better?
    void WebEscape(const std::string &source, std::string *output) {
        output->reserve(source.length() + 10);
        for (size_t pos = 0; pos != source.size(); ++pos) {
            switch (source[pos]) {
                case '&':
                    output->append("&amp;");
                    break;
                case '\"':
                    output->append("&quot;");
                    break;
                case '\'':
                    output->append("&apos;");
                    break;
                case '<':
                    output->append("&lt;");
                    break;
                case '>':
                    output->append("&gt;");
                    break;
                default:
                    output->push_back(source[pos]);
                    break;
            }
        }
    }

    void Controller::reset_sampled_request(SampledRequest *req) {
        delete _sampled_request;
        _sampled_request = req;
    }

    void Controller::set_stream_creator(StreamCreator *sc) {
        if (_stream_creator) {
            FLARE_LOG(FATAL) << "A StreamCreator has been set previously";
            return;
        }
        _stream_creator = sc;
    }

    flare::container::intrusive_ptr<ProgressiveAttachment>
    Controller::CreateProgressiveAttachment(StopStyle stop_style) {
        if (has_progressive_writer()) {
            FLARE_LOG(ERROR) << "One controller can only have one ProgressiveAttachment";
            return nullptr;
        }
        if (_request_protocol != PROTOCOL_HTTP) {
            FLARE_LOG(ERROR) << "Only http supports ProgressiveAttachment now";
            return nullptr;
        }
        if (_current_call.sending_sock == nullptr) {
            FLARE_LOG(ERROR) << "sending_sock is nullptr";
            return nullptr;
        }
        SocketUniquePtr httpsock;
        _current_call.sending_sock->ReAddress(&httpsock);

        if (stop_style == FORCE_STOP) {
            httpsock->fail_me_at_server_stop();
        }
        _wpa.reset(new ProgressiveAttachment(
                httpsock, http_request().before_http_1_1()));
        return _wpa;
    }

    void Controller::ReadProgressiveAttachmentBy(ProgressiveReader *r) {
        if (r == nullptr) {
            FLARE_LOG(FATAL) << "Param[r] is nullptr";
            return;
        }
        if (!is_response_read_progressively()) {
            return r->OnEndOfMessage(
                    flare::result_status(EINVAL, "Can't read progressive attachment from a "
                                                      "controller without calling "
                                                      "response_will_be_read_progressively() before"));
        }
        if (_rpa == nullptr) {
            return r->OnEndOfMessage(
                    flare::result_status(EINVAL, "ReadableProgressiveAttachment is nullptr"));
        }
        if (has_progressive_reader()) {
            return r->OnEndOfMessage(
                    flare::result_status(EPERM, "{} can't be called more than once",
                                              __FUNCTION__));
        }
        add_flag(FLAGS_PROGRESSIVE_READER);
        return _rpa->ReadProgressiveAttachmentBy(r);
    }

    void Controller::set_mongo_session_data(MongoContext *data) {
        _mongo_session_data = data;
    }

    bool Controller::is_ssl() const {
        Socket *s = _current_call.sending_sock.get();
        return s != nullptr && s->is_ssl();
    }

    x509_st *Controller::get_peer_certificate() const {
        Socket *s = _current_call.sending_sock.get();
        return s ? s->GetPeerCertificate() : nullptr;
    }

    int Controller::GetSockOption(int level, int optname, void *optval, socklen_t *optlen) {
        Socket *s = _current_call.sending_sock.get();
        if (s) {
            return getsockopt(s->fd(), level, optname, optval, optlen);
        } else {
            errno = EBADF;
            return -1;
        }
    }

#if defined(FLARE_PLATFORM_OSX)
    typedef sig_t SignalHandler;
#else
    typedef sighandler_t SignalHandler;
#endif

    static volatile bool s_signal_quit = false;
    static SignalHandler s_prev_sigint_handler = nullptr;
    static SignalHandler s_prev_sigterm_handler = nullptr;

    static void quit_handler(int signo) {
        s_signal_quit = true;
        if (SIGINT == signo && s_prev_sigint_handler) {
            s_prev_sigint_handler(signo);
        }
        if (SIGTERM == signo && s_prev_sigterm_handler) {
            s_prev_sigterm_handler(signo);
        }
    }

    static pthread_once_t register_quit_signal_once = PTHREAD_ONCE_INIT;

    static void RegisterQuitSignalOrDie() {
        // Not thread-safe.
        SignalHandler prev = signal(SIGINT, quit_handler);
        if (prev != SIG_DFL &&
            prev != SIG_IGN) { // shell may install SIGINT of background jobs with SIG_IGN
            if (prev == SIG_ERR) {
                FLARE_LOG(ERROR) << "Fail to register SIGINT, abort";
                abort();
            } else {
                s_prev_sigint_handler = prev;
                FLARE_LOG(WARNING) << "SIGINT was installed with " << prev;
            }
        }

        if (FLAGS_graceful_quit_on_sigterm) {
            prev = signal(SIGTERM, quit_handler);
            if (prev != SIG_DFL &&
                prev != SIG_IGN) { // shell may install SIGTERM of background jobs with SIG_IGN
                if (prev == SIG_ERR) {
                    FLARE_LOG(ERROR) << "Fail to register SIGTERM, abort";
                    abort();
                } else {
                    s_prev_sigterm_handler = prev;
                    FLARE_LOG(WARNING) << "SIGTERM was installed with " << prev;
                }
            }
        }
    }

    bool IsAskedToQuit() {
        pthread_once(&register_quit_signal_once, RegisterQuitSignalOrDie);
        return s_signal_quit;
    }

    void AskToQuit() {
        raise(SIGINT);
    }

    class DoNothingClosure : public google::protobuf::Closure {
        void Run() {}
    };

    google::protobuf::Closure *DoNothing() {
        return flare::base::get_leaky_singleton<DoNothingClosure>();
    }

    KVMap &Controller::SessionKV() {
        if (_session_kv == nullptr) {
            _session_kv.reset(new KVMap);
        }
        return *_session_kv.get();
    }

#define FLARE_RPC_SESSION_END_MSG "Session ends."
#define FLARE_RPC_REQ_ID "@rid"
#define FLARE_RPC_KV_SEP "="

    void Controller::FlushSessionKV(std::ostream &os) {
        if (_session_kv == nullptr || _session_kv->Count() == 0) {
            return;
        }

        const std::string *pRID = nullptr;
        if (!request_id().empty()) {
            pRID = &request_id();
        }

        if (FLAGS_flare_log_as_json) {
            if (pRID) {
                os << "\"" FLARE_RPC_REQ_ID "\":\"" << *pRID << "\",";
            }
            os << "\"M\":\"" FLARE_RPC_SESSION_END_MSG "\"";
            for (auto it = _session_kv->Begin(); it != _session_kv->End(); ++it) {
                os << ",\"" << it->first << "\":\"" << it->second << '"';
            }
        } else {
            if (pRID) {
                os << FLARE_RPC_REQ_ID FLARE_RPC_KV_SEP << *pRID << " ";
            }
            os << FLARE_RPC_SESSION_END_MSG;
            for (auto it = _session_kv->Begin(); it != _session_kv->End(); ++it) {
                os << ' ' << it->first << FLARE_RPC_KV_SEP << it->second;
            }
        }
    }

    std::ostream &operator<<(std::ostream &os, const Controller::LogPrefixDummy &p) {
        p.DoPrintLogPrefix(os);
        return os;
    }

    void Controller::DoPrintLogPrefix(std::ostream &os) const {
        const std::string *pRID = nullptr;
        if (!request_id().empty()) {
            pRID = &request_id();
            if (pRID) {
                if (FLAGS_flare_log_as_json) {
                    os << FLARE_RPC_REQ_ID "\":\"" << *pRID << "\",";
                } else {
                    os << FLARE_RPC_REQ_ID FLARE_RPC_KV_SEP << *pRID << " ";
                }
            }
        }
        if (FLAGS_flare_log_as_json) {
            os << "\"M\":\"";
        }
    }

} // namespace flare::rpc
