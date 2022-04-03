

#include "flare/thread/thread.h"
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

    __thread thread_impl *local_impl = nullptr;

    thread_impl::thread_impl(thread_option &&option)
            : option(std::move(option)), thread_id(0), start_latch(1) {}

    bool thread_impl::start() {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        if (!option.join_able) {
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            detached = true;
        }
        if (option.stack_size > 0) {
            pthread_attr_setstacksize(&attr, option.stack_size);
        }

        auto ret = ::pthread_create(&thread_id, &attr, &thread_impl::thread_func, this);
        pthread_attr_destroy(&attr);
        if (ret != 0) {
            return false;
        }
        start_latch.wait();
        return true;
    }

    void *thread_impl::thread_func(void *arg) {
        auto impl = ref_ptr(ref_ptr_v, (thread_impl *) arg);
        local_impl = impl.get();
        auto i = thread::thread_index();
        thread::set_name("%s#%d", impl->option.prefix.c_str(), i);
        LOG(INFO) << "start thread: " << impl->option.prefix << "#" << i;
        impl->set_affinity();
        impl->start_latch.count_down();
        impl->option.func();
        local_impl = nullptr;
        LOG(INFO) << "exit thread: " << impl->option.prefix << "#" << i;
        return nullptr;
    }

    void thread_impl::set_affinity() const {
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

    thread::~thread() {
        FLARE_CHECK(!_impl, "thread::join() was not called before destruction");
    }

    void thread::join() {
        if (!_impl) {
            return;
        }
        ::pthread_join(_impl->thread_id, nullptr);
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
        auto r = _impl->start();
        if (!_impl->option.join_able) {
            _impl = nullptr;
        }
        return r;
    }

    void thread::init_by_option(thread_option &&option) {
        _impl = ref_ptr(ref_ptr_v, new thread_impl(std::move(option)));
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

    namespace detail {

        class thread_exit_helper {
        public:
            ~thread_exit_helper() {
                // Call function reversely.
                while (!_fns.empty()) {
                    auto back = std::move(_fns.back());
                    _fns.pop_back();
                    // Notice that _fns may be changed after calling Fn.
                    if (back) {
                        back();
                    }
                }
            }

            size_t add(flare::function<void()> &&fn) {
                try {
                    if (_fns.capacity() < 16) {
                        _fns.reserve(16);
                    }
                    _fns.emplace_back(std::move(fn));
                } catch (...) {
                    errno = ENOMEM;
                    return -1;
                }
                return _fns.size() - 1;
            }

            void remove(size_t index) {
                if (index >= _fns.size()) {
                    return;
                }
                _fns[index] = nullptr;
            }

        private:
            std::vector<flare::function<void()>> _fns;
        };

        static pthread_key_t thread_atexit_key;
        static pthread_once_t thread_atexit_once = PTHREAD_ONCE_INIT;

        static void delete_thread_exit_helper(void *arg) {
            delete static_cast<thread_exit_helper *>(arg);
        }

        static void helper_exit_global() {
            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (h) {
                pthread_setspecific(detail::thread_atexit_key, nullptr);
                delete h;
            }
        }

        static void make_thread_atexit_key() {
            if (pthread_key_create(&thread_atexit_key, delete_thread_exit_helper) != 0) {
                fprintf(stderr, "Fail to create thread_atexit_key, abort\n");
                abort();
            }
            // If caller is not pthread, delete_thread_exit_helper will not be called.
            // We have to rely on atexit().
            atexit(helper_exit_global);
        }

        detail::thread_exit_helper *get_or_new_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);

            auto *h = (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (nullptr == h) {
                h = new(std::nothrow) detail::thread_exit_helper;
                if (nullptr != h) {
                    pthread_setspecific(detail::thread_atexit_key, h);
                }
            }
            return h;
        }

        detail::thread_exit_helper *get_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);
            return (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
        }

    }  // namespace detail


    bool thread::run_in_thread() const {
        if (_impl) {
            return pthread_self() == _impl->thread_id;
        }
        return false;
    }

    size_t thread::atexit(flare::function<void()> &&fn) {
        if (nullptr == fn) {
            errno = EINVAL;
            return -1;
        }
        detail::thread_exit_helper *h = detail::get_or_new_thread_exit_helper();
        if (h) {
            return h->add(std::move(fn));
        }
        errno = ENOMEM;
        return -1;
    }

    void thread::atexit_cancel(size_t index) {
        detail::thread_exit_helper *h = detail::get_thread_exit_helper();
        if (h) {
            h->remove(index);
        }
    }

    thread::native_handler_type thread::native_handler() {
        if (local_impl) {
            return local_impl->thread_id;
        }
        // in main thread or create not by flare thread
        pthread_t mid = pthread_self();
        return mid;
    }
}  // namespace flare
