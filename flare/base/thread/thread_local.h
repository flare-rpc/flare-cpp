

#ifndef FLARE_BASE_THREAD_THREAD_LOCAL_H_
#define FLARE_BASE_THREAD_THREAD_LOCAL_H_

#include <new>                      // std::nothrow
#include <cstddef>                  // nullptr
#include <cstdint>
#include "flare/base/profile.h"

#ifdef _MSC_VER
#define FLARE_THREAD_LOCAL __declspec(thread)
#else
#define FLARE_THREAD_LOCAL __thread
#endif  // _MSC_VER

namespace flare::base {

    // Get a thread-local object typed T. The object will be default-constructed
    // at the first call to this function, and will be deleted when thread
    // exits.
    template<typename T>
    inline T *get_thread_local();

    // |fn| or |fn(arg)| will be called at caller's exit. If caller is not a
    // thread, fn will be called at program termination. Calling sequence is LIFO:
    // last registered function will be called first. Duplication of functions
    // are not checked. This function is often used for releasing thread-local
    // resources declared with __thread which is much faster than
    // pthread_getspecific or boost::thread_specific_ptr.
    // Returns 0 on success, -1 otherwise and errno is set.
    int thread_atexit(void (*fn)());

    int thread_atexit(void (*fn)(void *), void *arg);

    // Remove registered function, matched functions will not be called.
    void thread_atexit_cancel(void (*fn)());

    void thread_atexit_cancel(void (*fn)(void *), void *arg);

    // Delete the typed-T object whose address is `arg'. This is a common function
    // to thread_atexit.
    template<typename T>
    void delete_object(void *arg) {
        delete static_cast<T *>(arg);
    }

    int32_t flare_tid();
}  // namespace flare::base

#include "flare/base/thread/thread_local_inl.h"

#endif  // FLARE_BASE_THREAD_THREAD_LOCAL_H_
