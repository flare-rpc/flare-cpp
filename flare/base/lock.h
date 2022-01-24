

#ifndef FLARE_BASE_LOCK_H_
#define FLARE_BASE_LOCK_H_

#include "flare/base/profile.h"

#if defined(FLARE_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(FLARE_PLATFORM_POSIX)

#include <pthread.h>

#endif

namespace flare::base {

// A convenient wrapper for an OS specific critical section.  
    class FLARE_EXPORT Mutex {
        FLARE_DISALLOW_COPY_AND_ASSIGN(Mutex);

    public:
#if defined(FLARE_PLATFORM_WINDOWS)
        typedef CRITICAL_SECTION NativeHandle;
#elif defined(FLARE_PLATFORM_POSIX)
        typedef pthread_mutex_t NativeHandle;
#endif

    public:
        Mutex() {
#if defined(FLARE_PLATFORM_WINDOWS)
            // The second parameter is the spin count, for short-held locks it avoid the
            // contending thread from going to sleep which helps performance greatly.
                ::InitializeCriticalSectionAndSpinCount(&_native_handle, 2000);
#elif defined(FLARE_PLATFORM_POSIX)
            pthread_mutex_init(&_native_handle, NULL);
#endif
        }

        ~Mutex() {
#if defined(FLARE_PLATFORM_WINDOWS)
            ::DeleteCriticalSection(&_native_handle);
#elif defined(FLARE_PLATFORM_POSIX)
            pthread_mutex_destroy(&_native_handle);
#endif
        }

        // Locks the mutex. If another thread has already locked the mutex, a call
        // to lock will block execution until the lock is acquired.
        void lock() {
#if defined(FLARE_PLATFORM_WINDOWS)
            ::EnterCriticalSection(&_native_handle);
#elif defined(FLARE_PLATFORM_POSIX)
            pthread_mutex_lock(&_native_handle);
#endif
        }

        // Unlocks the mutex. The mutex must be locked by the current thread of
        // execution, otherwise, the behavior is undefined.
        void unlock() {
#if defined(FLARE_PLATFORM_WINDOWS)
            ::LeaveCriticalSection(&_native_handle);
#elif defined(FLARE_PLATFORM_POSIX)
            pthread_mutex_unlock(&_native_handle);
#endif
        }

        // Tries to lock the mutex. Returns immediately.
        // On successful lock acquisition returns true, otherwise returns false.
        bool try_lock() {
#if defined(FLARE_PLATFORM_WINDOWS)
            return (::TryEnterCriticalSection(&_native_handle) != FALSE);
#elif defined(FLARE_PLATFORM_POSIX)
            return pthread_mutex_trylock(&_native_handle) == 0;
#endif
        }

        // Returns the underlying implementation-defined native handle object.
        NativeHandle *native_handle() { return &_native_handle; }

    private:
#if defined(FLARE_PLATFORM_POSIX)

        // The posix implementation of ConditionVariable needs to be able
        // to see our lock and tweak our debugging counters, as it releases
        // and acquires locks inside of pthread_cond_{timed,}wait.
        friend class ConditionVariable;

#elif defined(FLARE_PLATFORM_WINDOWS)
        // The Windows Vista implementation of ConditionVariable needs the
        // native handle of the critical section.
        friend class WinVistaCondVar;
#endif

        NativeHandle _native_handle;
    };

// TODO: Remove this type.
    class FLARE_EXPORT Lock : public Mutex {
        FLARE_DISALLOW_COPY_AND_ASSIGN(Lock);

    public:

        Lock() {}

        ~

        Lock() {}

        void Acquire() { lock(); }

        void Release() { unlock(); }

        bool Try() { return try_lock(); }

        void AssertAcquired() const {}
    };

// A helper class that acquires the given Lock while the AutoLock is in scope.
    class AutoLock {
    public:
        struct AlreadyAcquired {
        };

        explicit AutoLock(Lock &lock) : lock_(lock) {
            lock_.Acquire();
        }

        AutoLock(Lock &lock, const AlreadyAcquired &) : lock_(lock) {
            lock_.AssertAcquired();
        }

        ~AutoLock() {
            lock_.AssertAcquired();
            lock_.Release();
        }

    private:
        Lock &lock_;
        FLARE_DISALLOW_COPY_AND_ASSIGN(AutoLock);
    };

// AutoUnlock is a helper that will Release() the |lock| argument in the
// constructor, and re-Acquire() it in the destructor.
    class AutoUnlock {
    public:
        explicit AutoUnlock(Lock &lock) : lock_(lock) {
            // We require our caller to have the lock.
            lock_.AssertAcquired();
            lock_.Release();
        }

        ~AutoUnlock() {
            lock_.Acquire();
        }

    private:
        Lock &lock_;
        FLARE_DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
    };

}  // namespace flare::base

#endif  // FLARE_BASE_LOCK_H_
