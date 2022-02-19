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

// fiber - A M:N threading library to make applications more concurrent.

// Date: Sun, 22 Jan 2017

#ifndef BTHREAD_REMOTE_TASK_QUEUE_H
#define BTHREAD_REMOTE_TASK_QUEUE_H

#include "flare/container/bounded_queue.h"
#include "flare/base/profile.h"

namespace flare::fiber_internal {

class fiber_worker;

// A queue for storing bthreads created by non-workers. Since non-workers
// randomly choose a fiber_worker to push which distributes the contentions,
// this queue is simply implemented as a queue protected with a lock.
// The function names should be self-explanatory.
class RemoteTaskQueue {
public:
    RemoteTaskQueue() {}

    int init(size_t cap) {
        const size_t memsize = sizeof(fiber_id_t) * cap;
        void* q_mem = malloc(memsize);
        if (q_mem == NULL) {
            return -1;
        }
        flare::container::bounded_queue<fiber_id_t> q(q_mem, memsize, flare::container::OWNS_STORAGE);
        _tasks.swap(q);
        return 0;
    }

    bool pop(fiber_id_t* task) {
        if (_tasks.empty()) {
            return false;
        }
        _mutex.lock();
        const bool result = _tasks.pop(task);
        _mutex.unlock();
        return result;
    }

    bool push(fiber_id_t task) {
        _mutex.lock();
        const bool res = push_locked(task);
        _mutex.unlock();
        return res;
    }

    bool push_locked(fiber_id_t task) {
        return _tasks.push(task);
    }

    size_t capacity() const { return _tasks.capacity(); }
    
private:
friend class fiber_worker;
    FLARE_DISALLOW_COPY_AND_ASSIGN(RemoteTaskQueue);
    flare::container::bounded_queue<fiber_id_t> _tasks;
    flare::base::Mutex _mutex;
};

}  // namespace flare::fiber_internal

#endif  // BTHREAD_REMOTE_TASK_QUEUE_H
