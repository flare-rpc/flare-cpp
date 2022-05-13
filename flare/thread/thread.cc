

#include "flare/thread/thread.h"
#include "flare/base/profile.h"
#include "flare/log/logging.h"
#include "flare/bootstrap/bootstrap.h"
#include <signal.h>
#include <algorithm>  // std::sort
#include <unordered_set>
#include <cstdarg>
#include <cstdio>
#include <pthread.h>

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
        FLARE_LOG(INFO) << "start thread: " << impl->option.prefix << "#" << i;
        impl->set_affinity();
        impl->start_latch.count_down();
        impl->option.func();
        local_impl = nullptr;
        FLARE_LOG(INFO) << "exit thread: " << impl->option.prefix << "#" << i;
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
              CPU_SET(option.affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#elif defined(__FreeBSD__)
            cpuset_t cpuset;
            CPU_ZERO(&cpuset);
            for (size_t i = 0; i < count; i++) {
              CPU_SET(option.affinity[i].index, &cpuset);
            }
            auto thread = pthread_self();
            pthread_setaffinity_np(thread, sizeof(cpuset_t), &cpuset);
#else
        FLARE_CHECK(!flare::core_affinity::supported)<<"Attempting to use thread affinity on a unsupported platform";
#endif
    }

    thread::~thread() {
        if (_impl) {
            FLARE_LOG(WARNING) << "thread: " << _impl->option.prefix << "was not called before destruction, detach instead.";
            detach();
        }
    }

    void thread::join(void **ptr) {
        if (!_impl) {
            return;
        }
        ::pthread_join(_impl->thread_id, ptr);
        _impl = nullptr;
    }

    void thread::detach() {
        if (!_impl) {
            return;
        }
        ::pthread_detach(_impl->thread_id);
        _impl = nullptr;
    }

    void do_nothing_handler(int) {}

    static pthread_once_t register_sigurg_once = PTHREAD_ONCE_INIT;

    static void register_sigurg() {
        signal(SIGURG, do_nothing_handler);
    }

    void thread::kill() {
        if (!_impl) {
            return;
        }
        pthread_once(&register_sigurg_once, register_sigurg);
        kill(_impl->thread_id);
    }

    void thread::kill(pthread_t th) {
        pthread_once(&register_sigurg_once, register_sigurg);
        ::pthread_kill(th, SIGURG);
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
            typedef void (*Fn)(void *);

            typedef std::pair<Fn, void *> Pair;

            ~thread_exit_helper() {
                // Call function reversely.
                while (!_fns.empty()) {
                    Pair back = _fns.back();
                    _fns.pop_back();
                    // Notice that _fns may be changed after calling Fn.
                    back.first(back.second);
                }
            }

            int add(Fn fn, void *arg) {
                try {
                    if (_fns.capacity() < 16) {
                        _fns.reserve(16);
                    }
                    _fns.emplace_back(fn, arg);
                } catch (...) {
                    errno = ENOMEM;
                    return -1;
                }
                return 0;
            }

            void remove(Fn fn, void *arg) {
                std::vector<Pair>::iterator
                        it = std::find(_fns.begin(), _fns.end(), std::make_pair(fn, arg));
                if (it != _fns.end()) {
                    std::vector<Pair>::iterator ite = it + 1;
                    for (; ite != _fns.end() && ite->first == fn && ite->second == arg;
                           ++ite) {}
                    _fns.erase(it, ite);
                }
            }

        private:
            std::vector<Pair> _fns;
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
                pthread_setspecific(detail::thread_atexit_key, NULL);
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

            detail::thread_exit_helper *h =
                    (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
            if (NULL == h) {
                h = new(std::nothrow) detail::thread_exit_helper;
                if (NULL != h) {
                    pthread_setspecific(detail::thread_atexit_key, h);
                }
            }
            return h;
        }

        detail::thread_exit_helper *get_thread_exit_helper() {
            pthread_once(&detail::thread_atexit_once, detail::make_thread_atexit_key);
            return (detail::thread_exit_helper *) pthread_getspecific(detail::thread_atexit_key);
        }

        static void call_single_arg_fn(void *fn) {
            ((void (*)()) fn)();
        }

    }  // namespace detail


    bool thread::run_in_thread() const {
        if (_impl) {
            return pthread_self() == _impl->thread_id;
        }
        return false;
    }

    int thread::atexit(void (*fn)()) {
        if (NULL == fn) {
            errno = EINVAL;
            return -1;
        }
        return atexit(detail::call_single_arg_fn, (void *) fn);
    }


    int thread::atexit(void (*fn)(void *), void *arg) {
        if (NULL == fn) {
            errno = EINVAL;
            return -1;
        }
        detail::thread_exit_helper *h = detail::get_or_new_thread_exit_helper();
        if (h) {
            return h->add(fn, arg);
        }
        errno = ENOMEM;
        return -1;
    }

    void thread::atexit_cancel(void (*fn)()) {
        if (NULL != fn) {
            atexit_cancel(detail::call_single_arg_fn, (void *) fn);
        }
    }

    void thread::atexit_cancel(void (*fn)(void *), void *arg) {
        if (fn != NULL) {
            detail::thread_exit_helper *h = detail::get_thread_exit_helper();
            if (h) {
                h->remove(fn, arg);
            }
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
    // for main thread will index '0'
    FLARE_BOOTSTRAP(0, [] { thread::thread_index(); });

}  // namespace flare
