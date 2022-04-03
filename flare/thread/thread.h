
#ifndef FLARE_THREAD_THREAD_H_
#define FLARE_THREAD_THREAD_H_

#include <functional>
#include <vector>
#include <string>
#include <pthread.h>
#include "flare/thread/affinity.h"
#include "flare/base/functional.h"

namespace flare {

    // thread provides an OS abstraction for threads of execution.
    using thread_func = std::function<void()>;

    class thread_impl;

    struct thread_option {
        size_t stack_size{0};
        bool join_able{true};
        thread_func func;
        core_affinity affinity;
        std::string prefix;
    };

    class thread {
    public:
        typedef pthread_t native_handler_type;
        thread() = default;

        thread(thread &&) noexcept;

        thread &operator=(thread &&);

        // Start a new thread using the given affinity that calls func.
        explicit thread(const thread_option &option) {
            thread_option tmp = option;
            init_by_option(std::move(tmp));
        }

        thread(const core_affinity &affinity, const thread_func &func) {
            thread_option option;
            option.affinity = affinity;
            option.func = func;
            init_by_option(std::move(option));
        }

        thread(const std::string &prefix, const thread_func &func) {
            thread_option option;
            option.prefix = prefix;
            option.func = func;
            init_by_option(std::move(option));
        }

        thread(const std::string &prefix, const core_affinity &affinity, const thread_func &func) {
            thread_option option;
            option.prefix = prefix;
            option.affinity = affinity;
            option.func = func;
            init_by_option(std::move(option));
        }

        ~thread();

        bool start();

        // join() blocks until the thread completes.
        void join();

        // set_name() sets the name of the currently executing thread for displaying
        // in a debugger.
        static void set_name(const char *fmt, ...);

        static int32_t thread_index();

        static size_t atexit(flare::function<void()>&&);

        template<typename F, class T>
        static size_t atexit(F&&f, T && t) {
            return atexit([lf=std::forward<F>(f), lt = std::forward<T>(t)](){
                lf(lt);
            });
        }


        static void atexit_cancel(size_t index);

        static native_handler_type native_handler();

    private:
        void init_by_option(thread_option &&option);

        thread(const thread &) = delete;

        thread &operator=(const thread &) = delete;

        thread_impl *_impl{nullptr};
    };


}  // namespace flare

#endif  // FLARE_THREAD_THREAD_H_
