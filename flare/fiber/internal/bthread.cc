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

// Date: Tue Jul 10 17:40:58 CST 2012

#include <gflags/gflags.h>
#include "flare/log/logging.h"
#include "flare/fiber/internal/task_group.h"                // TaskGroup
#include "flare/fiber/internal/task_control.h"              // TaskControl
#include "flare/fiber/internal/timer_thread.h"
#include "flare/fiber/internal/list_of_abafree_id.h"
#include "flare/fiber/internal/bthread.h"

namespace flare::fiber_internal {

    DEFINE_int32(bthread_concurrency, 8 + BTHREAD_EPOLL_THREAD_NUM,
                 "Number of pthread workers");

    DEFINE_int32(bthread_min_concurrency, 0,
                 "Initial number of pthread workers which will be added on-demand."
                 " The laziness is disabled when this value is non-positive,"
                 " and workers will be created eagerly according to -bthread_concurrency and bthread_setconcurrency(). ");

    static bool never_set_bthread_concurrency = true;

    static bool validate_bthread_concurrency(const char *, int32_t val) {
        // bthread_setconcurrency sets the flag on success path which should
        // not be strictly in a validator. But it's OK for a int flag.
        return bthread_setconcurrency(val) == 0;
    }

    const int FLARE_ALLOW_UNUSED register_FLAGS_bthread_concurrency =
            ::GFLAGS_NS::RegisterFlagValidator(&FLAGS_bthread_concurrency,
                                               validate_bthread_concurrency);

    static bool validate_bthread_min_concurrency(const char *, int32_t val);

    const int FLARE_ALLOW_UNUSED register_FLAGS_bthread_min_concurrency =
            ::GFLAGS_NS::RegisterFlagValidator(&FLAGS_bthread_min_concurrency,
                                               validate_bthread_min_concurrency);

    static_assert(sizeof(TaskControl *) == sizeof(std::atomic<TaskControl *>), "atomic_size_match");

    pthread_mutex_t g_task_control_mutex = PTHREAD_MUTEX_INITIALIZER;
    // Referenced in rpc, needs to be extern.
    // Notice that we can't declare the variable as atomic<TaskControl*> which
    // are not constructed before main().
    TaskControl *g_task_control = NULL;

    extern FLARE_THREAD_LOCAL TaskGroup *tls_task_group;

    extern void (*g_worker_startfn)();

    inline TaskControl *get_task_control() {
        return g_task_control;
    }

    inline TaskControl *get_or_new_task_control() {
        std::atomic<TaskControl *> *p = (std::atomic<TaskControl *> *) &g_task_control;
        TaskControl *c = p->load(std::memory_order_consume);
        if (c != NULL) {
            return c;
        }
        FLARE_SCOPED_LOCK(g_task_control_mutex);
        c = p->load(std::memory_order_consume);
        if (c != NULL) {
            return c;
        }
        c = new(std::nothrow) TaskControl;
        if (NULL == c) {
            return NULL;
        }
        int concurrency = FLAGS_bthread_min_concurrency > 0 ?
                          FLAGS_bthread_min_concurrency :
                          FLAGS_bthread_concurrency;
        if (c->init(concurrency) != 0) {
            LOG(ERROR) << "Fail to init g_task_control";
            delete c;
            return NULL;
        }
        p->store(c, std::memory_order_release);
        return c;
    }

    static bool validate_bthread_min_concurrency(const char *, int32_t val) {
        if (val <= 0) {
            return true;
        }
        if (val < BTHREAD_MIN_CONCURRENCY || val > FLAGS_bthread_concurrency) {
            return false;
        }
        TaskControl *c = get_task_control();
        if (!c) {
            return true;
        }
        FLARE_SCOPED_LOCK(g_task_control_mutex);
        int concurrency = c->concurrency();
        if (val > concurrency) {
            int added = c->add_workers(val - concurrency);
            return added == (val - concurrency);
        } else {
            return true;
        }
    }

    __thread TaskGroup *tls_task_group_nosignal = NULL;

    FLARE_FORCE_INLINE int
    start_from_non_worker(bthread_t *__restrict tid,
                          const bthread_attr_t *__restrict attr,
                          void *(*fn)(void *),
                          void *__restrict arg) {
        TaskControl *c = get_or_new_task_control();
        if (NULL == c) {
            return ENOMEM;
        }
        if (attr != NULL && (attr->flags & BTHREAD_NOSIGNAL)) {
            // Remember the TaskGroup to insert NOSIGNAL tasks for 2 reasons:
            // 1. NOSIGNAL is often for creating many bthreads in batch,
            //    inserting into the same TaskGroup maximizes the batch.
            // 2. bthread_flush() needs to know which TaskGroup to flush.
            TaskGroup *g = tls_task_group_nosignal;
            if (NULL == g) {
                g = c->choose_one_group();
                tls_task_group_nosignal = g;
            }
            return g->start_background<true>(tid, attr, fn, arg);
        }
        return c->choose_one_group()->start_background<true>(
                tid, attr, fn, arg);
    }

    struct TidTraits {
        static const size_t BLOCK_SIZE = 63;
        static const size_t MAX_ENTRIES = 65536;
        static const bthread_t ID_INIT;

        static bool exists(bthread_t id) { return flare::fiber_internal::TaskGroup::exists(id); }
    };

    const bthread_t TidTraits::ID_INIT = INVALID_BTHREAD;

    typedef ListOfABAFreeId<bthread_t, TidTraits> TidList;

    struct TidStopper {
        void operator()(bthread_t id) const { bthread_stop(id); }
    };

    struct TidJoiner {
        void operator()(bthread_t &id) const {
            bthread_join(id, NULL);
            id = INVALID_BTHREAD;
        }
    };

}  // namespace flare::fiber_internal

extern "C" {

int bthread_start_urgent(bthread_t *__restrict tid,
                         const bthread_attr_t *__restrict attr,
                         void *(*fn)(void *),
                         void *__restrict arg) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (g) {
        // start from worker
        return flare::fiber_internal::TaskGroup::start_foreground(&g, tid, attr, fn, arg);
    }
    return flare::fiber_internal::start_from_non_worker(tid, attr, fn, arg);
}

int bthread_start_background(bthread_t *__restrict tid,
                             const bthread_attr_t *__restrict attr,
                             void *(*fn)(void *),
                             void *__restrict arg) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (g) {
        // start from worker
        return g->start_background<false>(tid, attr, fn, arg);
    }
    return flare::fiber_internal::start_from_non_worker(tid, attr, fn, arg);
}

void bthread_flush() {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (g) {
        return g->flush_nosignal_tasks();
    }
    g = flare::fiber_internal::tls_task_group_nosignal;
    if (g) {
        // NOSIGNAL tasks were created in this non-worker.
        flare::fiber_internal::tls_task_group_nosignal = NULL;
        return g->flush_nosignal_tasks_remote();
    }
}

int bthread_interrupt(bthread_t tid) {
    return flare::fiber_internal::TaskGroup::interrupt(tid, flare::fiber_internal::get_task_control());
}

int bthread_stop(bthread_t tid) {
    flare::fiber_internal::TaskGroup::set_stopped(tid);
    return bthread_interrupt(tid);
}

int bthread_stopped(bthread_t tid) {
    return (int) flare::fiber_internal::TaskGroup::is_stopped(tid);
}

bthread_t bthread_self(void) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    // note: return 0 for main tasks now, which include main thread and
    // all work threads. So that we can identify main tasks from logs
    // more easily. This is probably questionable in future.
    if (g != NULL && !g->is_current_main_task()/*note*/) {
        return g->current_tid();
    }
    return INVALID_BTHREAD;
}

int bthread_equal(bthread_t t1, bthread_t t2) {
    return t1 == t2;
}

void bthread_exit(void *retval) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (g != NULL && !g->is_current_main_task()) {
        throw flare::fiber_internal::ExitException(retval);
    } else {
        pthread_exit(retval);
    }
}

int bthread_join(bthread_t tid, void **thread_return) {
    return flare::fiber_internal::TaskGroup::join(tid, thread_return);
}

int bthread_attr_init(bthread_attr_t *a) {
    *a = BTHREAD_ATTR_NORMAL;
    return 0;
}

int bthread_attr_destroy(bthread_attr_t *) {
    return 0;
}

int bthread_getattr(bthread_t tid, bthread_attr_t *attr) {
    return flare::fiber_internal::TaskGroup::get_attr(tid, attr);
}

int bthread_getconcurrency(void) {
    return flare::fiber_internal::FLAGS_bthread_concurrency;
}

int bthread_setconcurrency(int num) {
    if (num < BTHREAD_MIN_CONCURRENCY || num > BTHREAD_MAX_CONCURRENCY) {
        LOG(ERROR) << "Invalid concurrency=" << num;
        return EINVAL;
    }
    if (flare::fiber_internal::FLAGS_bthread_min_concurrency > 0) {
        if (num < flare::fiber_internal::FLAGS_bthread_min_concurrency) {
            return EINVAL;
        }
        if (flare::fiber_internal::never_set_bthread_concurrency) {
            flare::fiber_internal::never_set_bthread_concurrency = false;
        }
        flare::fiber_internal::FLAGS_bthread_concurrency = num;
        return 0;
    }
    flare::fiber_internal::TaskControl *c = flare::fiber_internal::get_task_control();
    if (c != NULL) {
        if (num < c->concurrency()) {
            return EPERM;
        } else if (num == c->concurrency()) {
            return 0;
        }
    }
    FLARE_SCOPED_LOCK(flare::fiber_internal::g_task_control_mutex);
    c = flare::fiber_internal::get_task_control();
    if (c == NULL) {
        if (flare::fiber_internal::never_set_bthread_concurrency) {
            flare::fiber_internal::never_set_bthread_concurrency = false;
            flare::fiber_internal::FLAGS_bthread_concurrency = num;
        } else if (num > flare::fiber_internal::FLAGS_bthread_concurrency) {
            flare::fiber_internal::FLAGS_bthread_concurrency = num;
        }
        return 0;
    }
    if (flare::fiber_internal::FLAGS_bthread_concurrency != c->concurrency()) {
        LOG(ERROR) << "CHECK failed: bthread_concurrency="
                   << flare::fiber_internal::FLAGS_bthread_concurrency
                   << " != tc_concurrency=" << c->concurrency();
        flare::fiber_internal::FLAGS_bthread_concurrency = c->concurrency();
    }
    if (num > flare::fiber_internal::FLAGS_bthread_concurrency) {
        // Create more workers if needed.
        flare::fiber_internal::FLAGS_bthread_concurrency +=
                c->add_workers(num - flare::fiber_internal::FLAGS_bthread_concurrency);
        return 0;
    }
    return (num == flare::fiber_internal::FLAGS_bthread_concurrency ? 0 : EPERM);
}

int bthread_about_to_quit() {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (g != NULL) {
        flare::fiber_internal::fiber_entity *current_task = g->current_task();
        if (!(current_task->attr.flags & BTHREAD_NEVER_QUIT)) {
            current_task->about_to_quit = true;
        }
        return 0;
    }
    return EPERM;
}

int bthread_timer_add(bthread_timer_t *id, timespec abstime,
                      void (*on_timer)(void *), void *arg) {
    flare::fiber_internal::TaskControl *c = flare::fiber_internal::get_or_new_task_control();
    if (c == NULL) {
        return ENOMEM;
    }
    flare::fiber_internal::TimerThread *tt = flare::fiber_internal::get_or_create_global_timer_thread();
    if (tt == NULL) {
        return ENOMEM;
    }
    bthread_timer_t tmp = tt->schedule(on_timer, arg, abstime);
    if (tmp != 0) {
        *id = tmp;
        return 0;
    }
    return ESTOP;
}

int bthread_timer_del(bthread_timer_t id) {
    flare::fiber_internal::TaskControl *c = flare::fiber_internal::get_task_control();
    if (c != NULL) {
        flare::fiber_internal::TimerThread *tt = flare::fiber_internal::get_global_timer_thread();
        if (tt == NULL) {
            return EINVAL;
        }
        const int state = tt->unschedule(id);
        if (state >= 0) {
            return state;
        }
    }
    return EINVAL;
}

int bthread_usleep(uint64_t microseconds) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (NULL != g && !g->is_current_pthread_task()) {
        return flare::fiber_internal::TaskGroup::usleep(&g, microseconds);
    }
    return ::usleep(microseconds);
}

int bthread_yield(void) {
    flare::fiber_internal::TaskGroup *g = flare::fiber_internal::tls_task_group;
    if (NULL != g && !g->is_current_pthread_task()) {
        flare::fiber_internal::TaskGroup::yield(&g);
        return 0;
    }
    // pthread_yield is not available on MAC
    return sched_yield();
}

int bthread_set_worker_startfn(void (*start_fn)()) {
    if (start_fn == NULL) {
        return EINVAL;
    }
    flare::fiber_internal::g_worker_startfn = start_fn;
    return 0;
}

void bthread_stop_world() {
    flare::fiber_internal::TaskControl *c = flare::fiber_internal::get_task_control();
    if (c != NULL) {
        c->stop_and_join();
    }
}

int bthread_list_init(bthread_list_t *list,
                      unsigned /*size*/,
                      unsigned /*conflict_size*/) {
    list->impl = new(std::nothrow) flare::fiber_internal::TidList;
    if (NULL == list->impl) {
        return ENOMEM;
    }
    // Set unused fields to zero as well.
    list->head = 0;
    list->size = 0;
    list->conflict_head = 0;
    list->conflict_size = 0;
    return 0;
}

void bthread_list_destroy(bthread_list_t *list) {
    delete static_cast<flare::fiber_internal::TidList *>(list->impl);
    list->impl = NULL;
}

int bthread_list_add(bthread_list_t *list, bthread_t id) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    return static_cast<flare::fiber_internal::TidList *>(list->impl)->add(id);
}

int bthread_list_stop(bthread_list_t *list) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    static_cast<flare::fiber_internal::TidList *>(list->impl)->apply(flare::fiber_internal::TidStopper());
    return 0;
}

int bthread_list_join(bthread_list_t *list) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    static_cast<flare::fiber_internal::TidList *>(list->impl)->apply(flare::fiber_internal::TidJoiner());
    return 0;
}

}  // extern "C"