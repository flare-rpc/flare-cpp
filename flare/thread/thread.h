
#ifndef FLARE_THREAD_THREAD_H_
#define FLARE_THREAD_THREAD_H_

#include <functional>
#include <vector>
#include <string>
#include <pthread.h>
#include "flare/thread/affinity.h"
#include "flare/base/functional.h"
#include "flare/thread/latch.h"
#include "flare/memory/ref_ptr.h"

namespace flare {

    // thread provides an OS abstraction for threads of execution.
    using thread_func = flare::base::function<void()>;
    struct thread_option {
        size_t stack_size{8 * 1024 * 1024};
        bool join_able{true};
        thread_func func;
        core_affinity affinity;
        std::string prefix;
    };

    class thread_impl : public ref_counted<thread_impl> {
    public:
        explicit thread_impl(thread_option &&option);

        thread_option option;
        pthread_t thread_id;
        latch start_latch;
        std::atomic<bool> detached{false};

        bool start();

        static void *thread_func(void *arg);

        void set_affinity() const;
    };

    class thread {
    public:
        typedef pthread_t native_handler_type;

        thread() = default;

        thread(const thread &) = delete;

        thread &operator=(const thread &) = delete;

        thread(thread &&) noexcept;


        thread &operator=(thread &&);

        // Start a new thread using the given affinity that calls func.
        explicit thread(thread_option &&option) {
            thread_option tmp = std::forward<thread_option>(option);
            init_by_option(std::move(tmp));
        }

        thread(const core_affinity &affinity, thread_func &&func) {
            thread_option option;
            option.affinity = affinity;
            option.func = std::forward<thread_func>(func);
            init_by_option(std::move(option));
        }

        thread(const std::string &prefix, thread_func &&func) {
            thread_option option;
            option.prefix = prefix;
            option.func = std::forward<thread_func>(func);
            init_by_option(std::move(option));
        }

        thread(const std::string &prefix, const core_affinity &affinity, thread_func &&func) {
            thread_option option;
            option.prefix = prefix;
            option.affinity = affinity;
            option.func = std::forward<thread_func>(func);
            init_by_option(std::move(option));
        }

        ~thread();

        bool start();

        // join() blocks until the thread completes.
        void join(void**ptr = nullptr);

        void detach();

        void kill();

        bool run_in_thread() const;

        // set_name() sets the name of the currently executing thread for displaying
        // in a debugger.
        static void set_name(const char *fmt, ...);

        static void kill(pthread_t th);

        static int32_t thread_index();

        static int atexit(void (*fn)(void *), void *arg);

        static int atexit(void (*fn)());

        static void atexit_cancel(void (*fn)());

        static void atexit_cancel(void (*fn)(void *), void *arg);

        static native_handler_type native_handler();

    private:
        void init_by_option(thread_option &&option);

        flare::ref_ptr<thread_impl> _impl = nullptr;
    };


}  // namespace flare

#endif  // FLARE_THREAD_THREAD_H_
