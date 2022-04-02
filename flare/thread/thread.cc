

#include "flare/thread/thread.h"
#include "flare/thread/latch.h"
#include "flare/base/profile.h"
#include "flare/log/logging.h"
#include <algorithm>  // std::sort
#include <unordered_set>
#include <cstdarg>
#include <cstdio>

#if defined(__APPLE__)


#include <mach/thread_act.h>
#include <pthread.h>
#include <unistd.h>
#include <thread>

#elif defined(__FreeBSD__)
#include <pthread.h>
#include <pthread_np.h>
#include <unistd.h>
#include <thread>
#else
#include <pthread.h>
#include <unistd.h>
#include <thread>
#endif

namespace flare {

    class thread::thread_impl :public ref_counted<thread::thread_impl>{
    public:
        explicit thread_impl(thread_option &&option)
                : option(std::move(option)), thread_id(0), start_latch(1) {}

        thread_option option;
        pthread_t thread_id;
        latch start_latch;
        std::atomic<bool> detached{false};

        bool start() {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            if (!option.join_able) {
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                detached = true;
            }
            if (option.stack_size > 0) {
                pthread_attr_setstacksize(&attr, option.stack_size);
            }

            auto ret = ::pthread_create(&thread_id, &attr, &thread::thread_impl::thread_func, this);
            pthread_attr_destroy(&attr);
            if (ret != 0) {
                return false;
            }
            start_latch.wait();
            return true;
        }

        static void *thread_func(void *arg) {
            auto impl = ref_ptr<thread::thread_impl>(ref_ptr_v, (thread::thread_impl *) arg);
            auto i = thread::thread_index();
            thread::set_name("%s#%d", impl->option.prefix.c_str(), i);
            impl->set_affinity();
            impl->start_latch.count_down();
            impl->option.func();
            return nullptr;
        }

        void set_affinity() const {
            auto count = option.affinity.count();
            if (count == 0) {
                return;
            }

#if defined(__linux__) && !defined(__ANDROID__)
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                for (size_t i = 0; i < count; i++) {
                  CPU_SET(affinity[i].index, &cpuset);
                }
                auto thread = pthread_self();
                pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#elif defined(__FreeBSD__)
                cpuset_t cpuset;
                CPU_ZERO(&cpuset);
                for (size_t i = 0; i < count; i++) {
                  CPU_SET(affinity[i].index, &cpuset);
                }
                auto thread = pthread_self();
                pthread_setaffinity_np(thread, sizeof(cpuset_t), &cpuset);
#else
            FLARE_CHECK(!flare::core_affinity::supported,
                        "Attempting to use thread affinity on a unsupported platform");
#endif
        }
    };

    thread::~thread() {
        FLARE_CHECK(!_impl, "thread::join() was not called before destruction");
    }

    void thread::join() {
        bool old;
        auto r = _impl->detached.compare_exchange_weak(old, true, std::memory_order_acquire);
        if (r && !old) {
            ::pthread_join(_impl->thread_id, nullptr);
        }
        _impl = nullptr;
    }

    void thread::set_name(const char *fmt, ...) {
        char name[1024];
        va_list vararg;
        va_start(vararg, fmt);
        vsnprintf(name, sizeof(name), fmt, vararg);
        va_end(vararg);

#if defined(__APPLE__)
        pthread_setname_np(name);
#elif defined(__FreeBSD__)
        pthread_set_name_np(pthread_self(), name);
#elif !defined(__Fuchsia__)
        pthread_setname_np(pthread_self(), name);
#endif

    }

    bool thread::start() {
        FLARE_CHECK(_impl);
        return _impl->start();
    }

    void thread::init_by_option(thread_option &&option) {
        _impl = ref_ptr(ref_ptr_v, new thread::thread_impl(std::move(option)));
    }


    thread::thread(thread &&rhs) noexcept: _impl(std::move(rhs._impl)) {
        rhs._impl = nullptr;
    }

    thread &thread::operator=(thread &&rhs) {
        _impl = rhs._impl;
        rhs._impl = nullptr;
        return *this;
    }

    std::atomic<int32_t> g_thread_id{0};
    __thread int32_t local_thread_id = -1;

    int32_t thread::thread_index() {
        if (FLARE_UNLIKELY(local_thread_id == -1)) {
            local_thread_id = g_thread_id.fetch_add(1, std::memory_order_relaxed);
        }
        return local_thread_id;
    }
}  // namespace flare
