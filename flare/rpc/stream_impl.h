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


#ifndef  FLARE_RPC_STREAM_IMPL_H_
#define  FLARE_RPC_STREAM_IMPL_H_

#include "flare/fiber/internal/fiber.h"
#include "flare/fiber/internal/execution_queue.h"
#include "flare/rpc/socket.h"
#include "flare/rpc/stream.h"
#include "flare/rpc/streaming_rpc_meta.pb.h"

namespace flare::rpc {

class FLARE_CACHELINE_ALIGNMENT Stream : public SocketConnection {
public:
    // |--------------------------------------------------|
    // |----------- Implement SocketConnection -----------|
    // |--------------------------------------------------|
   
    int Connect(Socket* ptr, const timespec* due_time,
                int (*on_connect)(int, int, void *), void *data);
    ssize_t CutMessageIntoFileDescriptor(int, flare::cord_buf **data_list,
                                         size_t size);
    ssize_t CutMessageIntoSSLChannel(SSL*, flare::cord_buf**, size_t);
    void BeforeRecycle(Socket *);

    // --------------------- SocketConnection --------------

    int AppendIfNotFull(const flare::cord_buf& msg);
    static int Create(const StreamOptions& options,
                      const StreamSettings *remote_settings,
                      StreamId *id);
    StreamId id() { return _id; }

    int OnReceived(const StreamFrameMeta& fm, flare::cord_buf *buf, Socket* sock);
    void SetRemoteSettings(const StreamSettings& remote_settings) {
        _remote_settings.MergeFrom(remote_settings);
    }
    int SetHostSocket(Socket *host_socket);
    void SetConnected();
    void SetConnected(const StreamSettings *remote_settings);

    void Wait(void (*on_writable)(StreamId, void*, int), void *arg,
                    const timespec *due_time);
    int Wait(const timespec* due_time);
    void FillSettings(StreamSettings *settings);
    static int SetFailed(StreamId id);
    void Close();

private:
friend void StreamWait(StreamId stream_id, const timespec *due_time,
                void (*on_writable)(StreamId, void*, int), void *arg);
friend class MessageBatcher;
    Stream();
    ~Stream();
    int Init(const StreamOptions options);
    void SetRemoteConsumed(size_t _remote_consumed);
    void TriggerOnConnectIfNeed();
    void Wait(void (*on_writable)(StreamId, void*, int), void* arg, 
              const timespec* due_time, bool new_thread, fiber_token_t *join_id);
    void SendFeedback();
    void StartIdleTimer();
    void StopIdleTimer();
    void HandleRpcResponse(flare::cord_buf* response_buffer);
    void WriteToHostSocket(flare::cord_buf* b);

    static int Consume(void *meta, flare::fiber_internal::TaskIterator<flare::cord_buf*>& iter);
    static int TriggerOnWritable(fiber_token_t id, void *data, int error_code);
    static void *RunOnWritable(void* arg);
    static void* RunOnConnect(void* arg);

    struct ConnectMeta {
        int (*on_connect)(int, int, void*);
        int ec;
        void* arg;
    };

    struct WritableMeta {
        void (*on_writable)(StreamId, void*, int);
        StreamId id;
        void *arg;
        int error_code;
        bool new_thread;
        bool has_timer;
        fiber_timer_id timer;
    };

    Socket*     _host_socket;  // Every stream within a Socket holds a reference
    Socket*     _fake_socket_weak_ref;  // Not holding reference
    StreamId    _id;
    StreamOptions _options;

    fiber_mutex_t     _connect_mutex;
    ConnectMeta         _connect_meta;
    bool                _connected;
    bool                _closed;
    
    fiber_mutex_t _congestion_control_mutex;
    size_t _produced;
    size_t _remote_consumed;
    fiber_token_list_t _writable_wait_list;

    int64_t _local_consumed;
    StreamSettings _remote_settings;   

    bool _parse_rpc_response;
    flare::fiber_internal::ExecutionQueueId<flare::cord_buf*> _consumer_queue;
    flare::cord_buf *_pending_buf;
    int64_t _start_idle_timer_us;
    fiber_timer_id _idle_timer;
};

} // namespace flare::rpc



#endif  // FLARE_RPC_STREAM_IMPL_H_
