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


#ifndef FLARE_RPC_EVENT_DISPATCHER_H_
#define FLARE_RPC_EVENT_DISPATCHER_H_

#include "flare/base/profile.h"                     // FLARE_DISALLOW_COPY_AND_ASSIGN
#include "flare/fiber/internal/types.h"                   // fiber_id_t, fiber_attribute
#include "flare/rpc/socket.h"                     // Socket, SocketId


namespace flare::rpc {

    // Dispatch edge-triggered events of file descriptors to consumers
    // running in separate fibers.
    class EventDispatcher {
        friend class Socket;

    public:
        EventDispatcher();

        virtual ~EventDispatcher();

        // Start this dispatcher in a fiber.
        // Use |*consumer_thread_attr| (if it's not NULL) as the attribute to
        // create fibers running user callbacks.
        // Returns 0 on success, -1 otherwise.
        virtual int Start(const fiber_attribute *consumer_thread_attr);

        // True iff this dispatcher is running in a fiber
        bool Running() const;

        // Stop fiber of this dispatcher.
        void Stop();

        // Suspend calling thread until fiber of this dispatcher stops.
        void Join();

        // When edge-triggered events happen on `fd', call
        // `on_edge_triggered_events' of `socket_id'.
        // Notice that this function also transfers ownership of `socket_id',
        // When the file descriptor is removed from internal epoll, the Socket
        // will be dereferenced once additionally.
        // Returns 0 on success, -1 otherwise.
        int AddConsumer(SocketId socket_id, int fd);

        // Watch EPOLLOUT event on `fd' into epoll device. If `pollin' is
        // true, EPOLLIN event will also be included and EPOLL_CTL_MOD will
        // be used instead of EPOLL_CTL_ADD. When event arrives,
        // `Socket::HandleEpollOut' will be called with `socket_id'
        // Returns 0 on success, -1 otherwise and errno is set
        int AddEpollOut(SocketId socket_id, int fd, bool pollin);

        // Remove EPOLLOUT event on `fd'. If `pollin' is true, EPOLLIN event
        // will be kept and EPOLL_CTL_MOD will be used instead of EPOLL_CTL_DEL
        // Returns 0 on success, -1 otherwise and errno is set
        int RemoveEpollOut(SocketId socket_id, int fd, bool pollin);

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(EventDispatcher);

        // Calls Run()
        static void *RunThis(void *arg);

        // Thread entry.
        void Run();

        // Remove the file descriptor `fd' from epoll.
        int RemoveConsumer(int fd);

        // The epoll to watch events.
        int _epfd;

        // false unless Stop() is called.
        volatile bool _stop;

        // identifier of hosting fiber
        fiber_id_t _tid;

        // The attribute of fibers calling user callbacks.
        fiber_attribute _consumer_thread_attr;

        // The attribute of fiber epoll_wait.
        fiber_attribute _epoll_thread_attr;

        // Pipe fds to wakeup EventDispatcher from `epoll_wait' in order to quit
        int _wakeup_fds[2];
    };

    EventDispatcher &GetGlobalEventDispatcher(int fd);

} // namespace flare::rpc


#endif  // FLARE_RPC_EVENT_DISPATCHER_H_
