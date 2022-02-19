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

#include <sys/uio.h>               // writev
#include "flare/base/compat.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include "flare/base/time.h"
#include "flare/base/scoped_lock.h"
#include "flare/base/fd_utility.h"
#include "flare/log/logging.h"
#include "flare/base/gperftools_profiler.h"
#include "flare/fiber/internal/bthread.h"
#include "flare/fiber/internal/task_control.h"
#include "flare/fiber/internal/task_group.h"
#include "flare/fiber/this_fiber.h"

#if defined(FLARE_PLATFORM_OSX)
#include <sys/types.h>                           // struct kevent
#include <sys/event.h>                           // kevent(), kqueue()
#endif

#define RUN_EPOLL_IN_BTHREAD

namespace flare::fiber_internal {
extern TaskControl* global_task_control;
int stop_and_join_epoll_threads();
}

namespace {
volatile bool client_stop = false;
volatile bool server_stop = false;

struct FLARE_CACHELINE_ALIGNMENT ClientMeta {
    int fd;
    size_t times;
    size_t bytes;
};

struct FLARE_CACHELINE_ALIGNMENT SocketMeta {
    int fd;
    int epfd;
    std::atomic<int> req;
    char* buf;
    size_t buf_cap;
    size_t bytes;
    size_t times;
};

struct EpollMeta {
    int epfd;
    int nthread;
    int nfold;
};

void* process_thread(void* arg) {
    SocketMeta* m = (SocketMeta*)arg;
    do {
        // Read all data.
        do {
            ssize_t n = read(m->fd, m->buf, m->buf_cap);
            if (n > 0) {
                m->bytes += n;
                ++m->times;
                if ((size_t)n < m->buf_cap) {
                    break;
                }
            } else if (n < 0) {
                if (errno == EAGAIN) {
                    break;
                } else if (errno == EINTR) {
                    continue;
                } else {
                    PLOG(FATAL) << "Fail to read fd=" << m->fd;
                    return NULL;
                }
            } else {
                LOG(FATAL) << "Another end closed fd=" << m->fd;
                return NULL;
            }
        } while (1);
        
        if (m->req.exchange(0, std::memory_order_release) == 1) {
            // no events during reading.
            break;
        }
        if (m->req.fetch_add(1, std::memory_order_relaxed) != 0) {
            // someone else takes the fd.
            break;
        }
    } while (1);
    return NULL;
}

void* epoll_thread(void* arg) {    
    EpollMeta* em = (EpollMeta*)arg;
    em->nthread = 0;
    em->nfold = 0;
#if defined(FLARE_PLATFORM_LINUX)
    epoll_event e[32];
#elif defined(FLARE_PLATFORM_OSX)
    struct kevent e[32];
#endif

    while (!server_stop) {
#if defined(FLARE_PLATFORM_LINUX)
        const int n = epoll_wait(em->epfd, e, FLARE_ARRAY_SIZE(e), -1);
#elif defined(FLARE_PLATFORM_OSX)
        const int n = kevent(em->epfd, NULL, 0, e, FLARE_ARRAY_SIZE(e), NULL);
#endif
        if (server_stop) {
            break;
        }
        if (n < 0) {
            if (EINTR == errno) {
                continue;
            }
#if defined(FLARE_PLATFORM_LINUX)
            PLOG(FATAL) << "Fail to epoll_wait";
#elif defined(FLARE_PLATFORM_OSX)
            PLOG(FATAL) << "Fail to kevent";
#endif
            break;
        }

        for (int i = 0; i < n; ++i) {
#if defined(FLARE_PLATFORM_LINUX)
            SocketMeta* m = (SocketMeta*)e[i].data.ptr;
#elif defined(FLARE_PLATFORM_OSX)
            SocketMeta* m = (SocketMeta*)e[i].udata;
#endif
            if (m->req.fetch_add(1, std::memory_order_acquire) == 0) {
                bthread_t th;
                bthread_start_urgent(
                    &th, &BTHREAD_ATTR_SMALL, process_thread, m);
                ++em->nthread;
            } else {
                ++em->nfold;
            }
        }
    }
    return NULL;
}

void* client_thread(void* arg) {
    ClientMeta* m = (ClientMeta*)arg;
    size_t offset = 0;
    m->times = 0;
    m->bytes = 0;
    const size_t buf_cap = 32768;
    char* buf = (char*)malloc(buf_cap);
    for (size_t i = 0; i < buf_cap/8; ++i) {
        ((uint64_t*)buf)[i] = i;
    }
    while (!client_stop) {
        ssize_t n;
        if (offset == 0) {
            n = write(m->fd, buf, buf_cap);
        } else {
            iovec v[2];
            v[0].iov_base = buf + offset;
            v[0].iov_len = buf_cap - offset;
            v[1].iov_base = buf;
            v[1].iov_len = offset;
            n = writev(m->fd, v, 2);
        }
        if (n < 0) {
            if (errno != EINTR) {
                PLOG(FATAL) << "Fail to write fd=" << m->fd;
                return NULL;
            }
        } else {
            ++m->times;
            m->bytes += n;
            offset += n;
            if (offset >= buf_cap) {
                offset -= buf_cap;
            }
        }
    }
    return NULL;
}

inline uint32_t fmix32 ( uint32_t h ) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

TEST(DispatcherTest, dispatch_tasks) {
    client_stop = false;
    server_stop = false;

    const size_t NEPOLL = 1;
    const size_t NCLIENT = 16;

    int epfd[NEPOLL];
    bthread_t eth[NEPOLL];
    EpollMeta* em[NEPOLL];
    int fds[2 * NCLIENT];
    pthread_t cth[NCLIENT];
    ClientMeta* cm[NCLIENT];
    SocketMeta* sm[NCLIENT];

    for (size_t i = 0; i < NEPOLL; ++i) {
#if defined(FLARE_PLATFORM_LINUX)
        epfd[i] = epoll_create(1024);
#elif defined(FLARE_PLATFORM_OSX)
        epfd[i] = kqueue();
#endif
        ASSERT_GT(epfd[i], 0);
    }
    
    for (size_t i = 0; i < NCLIENT; ++i) {
        ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds + 2 * i));
        SocketMeta* m = new SocketMeta;
        m->fd = fds[i * 2];
        m->epfd = epfd[fmix32(i) % NEPOLL];
        m->req = 0;
        m->buf_cap = 32768;
        m->buf = (char*)malloc(m->buf_cap);
        m->bytes = 0;
        m->times = 0;
        ASSERT_EQ(0, flare::base::make_non_blocking(m->fd));
        sm[i] = m;

#if defined(FLARE_PLATFORM_LINUX)
        epoll_event evt = { (uint32_t)(EPOLLIN | EPOLLET), { m } };
        ASSERT_EQ(0, epoll_ctl(m->epfd, EPOLL_CTL_ADD, m->fd, &evt));
#elif defined(FLARE_PLATFORM_OSX)
        struct kevent kqueue_event;
        EV_SET(&kqueue_event, m->fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, m);
        ASSERT_EQ(0, kevent(m->epfd, &kqueue_event, 1, NULL, 0, NULL));
#endif

        cm[i] = new ClientMeta;
        cm[i]->fd = fds[i * 2 + 1];
        cm[i]->times = 0;
        cm[i]->bytes = 0;
        ASSERT_EQ(0, pthread_create(&cth[i], NULL, client_thread, cm[i]));
    }
    
    ProfilerStart("dispatcher.prof");
    flare::base::stop_watcher tm;
    tm.start();

    for (size_t i = 0; i < NEPOLL; ++i) {
        EpollMeta *m = new EpollMeta;
        em[i] = m;
        m->epfd = epfd[i];
#ifdef RUN_EPOLL_IN_BTHREAD
        ASSERT_EQ(0, bthread_start_background(&eth[i], NULL, epoll_thread, m));
#else
        ASSERT_EQ(0, pthread_create(&eth[i], NULL, epoll_thread, m));
#endif
    }

    sleep(5);

    tm.stop();
    ProfilerStop();
    size_t client_bytes = 0;
    size_t server_bytes = 0;
    for (size_t i = 0; i < NCLIENT; ++i) {
        client_bytes += cm[i]->bytes;
        server_bytes += sm[i]->bytes;
    }
    size_t all_nthread = 0, all_nfold = 0;
    for (size_t i = 0; i < NEPOLL; ++i) {
        all_nthread += em[i]->nthread;
        all_nfold += em[i]->nfold;
    }

    LOG(INFO) << "client_tp=" << client_bytes / (double)tm.u_elapsed()
              << "MB/s server_tp=" << server_bytes / (double)tm.u_elapsed()
              << "MB/s nthread=" << all_nthread << " nfold=" << all_nfold;

    client_stop = true;
    for (size_t i = 0; i < NCLIENT; ++i) {
        pthread_join(cth[i], NULL);
    }
    server_stop = true;
    for (size_t i = 0; i < NEPOLL; ++i) {
#if defined(FLARE_PLATFORM_LINUX)
        epoll_event evt = { EPOLLOUT,  { NULL } };
        ASSERT_EQ(0, epoll_ctl(epfd[i], EPOLL_CTL_ADD, 0, &evt));
#elif defined(FLARE_PLATFORM_OSX)
        struct kevent kqueue_event;
        EV_SET(&kqueue_event, 0, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
        ASSERT_EQ(0, kevent(epfd[i], &kqueue_event, 1, NULL, 0, NULL));
#endif
#ifdef RUN_EPOLL_IN_BTHREAD
        bthread_join(eth[i], NULL);
#else
        pthread_join(eth[i], NULL);
#endif
    }
    flare::fiber_internal::stop_and_join_epoll_threads();
    flare::this_fiber::fiber_sleep_for(100000);
}
} // namespace
