
#ifndef BUTIL_LAZY_INSTANCE_H_
#define BUTIL_LAZY_INSTANCE_H_

#include <new>  // For placement new.

#include "flare/base/static_atomic.h"
#include "flare/butil/base_export.h"
#include "flare/butil/basictypes.h"
#include "flare/butil/debug/leak_annotations.h"
#include "flare/base/logging.h"
#include "flare/butil/memory/aligned_memory.h"
#include "flare/base/dynamic_annotations/dynamic_annotations.h"
#include "flare/butil/threading/thread_restrictions.h"

// LazyInstance uses its own struct initializer-list style static
// initialization, as base's LINKER_INITIALIZED requires a constructor and on
// some compilers (notably gcc 4.4) this still ends up needing runtime
// initialization.
#define LAZY_INSTANCE_INITIALIZER { 0, {{0}} }

namespace butil {

    template<typename Type>
    struct DefaultLazyInstanceTraits {
        static const bool kRegisterOnExit;
#ifndef NDEBUG
        static const bool kAllowedToAccessOnNonjoinableThread;
#endif

        static Type *New(void *instance) {
            DCHECK_EQ(reinterpret_cast<uintptr_t>(instance) & (ALIGNOF(Type) - 1), 0u)
                    << ": Bad boy, the buffer passed to placement new is not aligned!\n"
                       "This may break some stuff like SSE-based optimizations assuming the "
                       "<Type> objects are word aligned.";
            // Use placement new to initialize our instance in our preallocated space.
            // The parenthesis is very important here to force POD type initialization.
            return new(instance) Type();
        }

        static void Delete(Type *instance) {
            // Explicitly call the destructor.
            instance->~Type();
        }
    };

// NOTE(gejun): BullseyeCoverage Compile C++ 8.4.4 complains about `undefined
// reference' on in-place assignments to static constants.
    template<typename Type>
    const bool DefaultLazyInstanceTraits<Type>::kRegisterOnExit = true;
#ifndef NDEBUG
    template <typename Type>
    const bool DefaultLazyInstanceTraits<Type>::kAllowedToAccessOnNonjoinableThread = false;
#endif

// We pull out some of the functionality into non-templated functions, so we
// can implement the more complicated pieces out of line in the .cc file.
    namespace internal {

// Use LazyInstance<T>::Leaky for a less-verbose call-site typedef; e.g.:
// butil::LazyInstance<T>::Leaky my_leaky_lazy_instance;
// instead of:
// butil::LazyInstance<T, butil::internal::LeakyLazyInstanceTraits<T> >
// my_leaky_lazy_instance;
// (especially when T is MyLongTypeNameImplClientHolderFactory).
// Only use this internal::-qualified verbose form to extend this traits class
// (depending on its implementation details).
        template<typename Type>
        struct LeakyLazyInstanceTraits {
            static const bool kRegisterOnExit;
#ifndef NDEBUG
            static const bool kAllowedToAccessOnNonjoinableThread;
#endif

            static Type *New(void *instance) {
                ANNOTATE_SCOPED_MEMORY_LEAK;
                return DefaultLazyInstanceTraits<Type>::New(instance);
            }

            static void Delete(Type *instance) {
            }
        };

        template<typename Type>
        const bool LeakyLazyInstanceTraits<Type>::kRegisterOnExit = false;
#ifndef NDEBUG
        template <typename Type>
        const bool LeakyLazyInstanceTraits<Type>::kAllowedToAccessOnNonjoinableThread = true;
#endif

// Our AtomicWord doubles as a spinlock, where a value of
// kBeingCreatedMarker means the spinlock is being held for creation.
        static const intptr_t kLazyInstanceStateCreating = 1;

// Check if instance needs to be created. If so return true otherwise
// if another thread has beat us, wait for instance to be created and
// return false.
        BUTIL_EXPORT bool NeedsLazyInstance(std::atomic<intptr_t> *state);

// After creating an instance, call this to register the dtor to be called
// at program exit and to update the atomic state to hold the |new_instance|
        BUTIL_EXPORT void CompleteLazyInstance(std::atomic<intptr_t> *state,
                                               intptr_t new_instance,
                                               void *lazy_instance,
                                               void (*dtor)(void *));

    }  // namespace internal

    template<typename Type, typename Traits = DefaultLazyInstanceTraits<Type> >
    class LazyInstance {
    public:
        // Do not define a destructor, as doing so makes LazyInstance a
        // non-POD-struct. We don't want that because then a static initializer will
        // be created to register the (empty) destructor with atexit() under MSVC, for
        // example. We handle destruction of the contained Type class explicitly via
        // the OnExit member function, where needed.
        // ~LazyInstance() {}

        // Convenience typedef to avoid having to repeat Type for leaky lazy
        // instances.
        typedef LazyInstance<Type, internal::LeakyLazyInstanceTraits<Type> > Leaky;

        Type &Get() {
            return *Pointer();
        }

        Type *Pointer() {
#ifndef NDEBUG
            // Avoid making TLS lookup on release builds.
            if (!Traits::kAllowedToAccessOnNonjoinableThread)
              ThreadRestrictions::AssertSingletonAllowed();
#endif
            // If any bit in the created mask is true, the instance has already been
            // fully constructed.
            static const intptr_t kLazyInstanceCreatedMask =
                    ~internal::kLazyInstanceStateCreating;

            // We will hopefully have fast access when the instance is already created.
            // Since a thread sees private_instance_ == 0 or kLazyInstanceStateCreating
            // at most once, the load is taken out of NeedsInstance() as a fast-path.
            // The load has acquire memory ordering as a thread which sees
            // private_instance_ > creating needs to acquire visibility over
            // the associated data (private_buf_). Pairing Release_Store is in
            // CompleteLazyInstance().
            intptr_t value = private_instance_.load();
            if (!(value & kLazyInstanceCreatedMask) &&
                internal::NeedsLazyInstance(&private_instance_)) {
                // Create the instance in the space provided by |private_buf_|.
                value = reinterpret_cast<intptr_t>(
                        Traits::New(private_buf_.void_data()));
                internal::CompleteLazyInstance(&private_instance_, value, this,
                                               Traits::kRegisterOnExit ? OnExit : NULL);
            }

            // This annotation helps race detectors recognize correct lock-less
            // synchronization between different threads calling Pointer().
            // We suggest dynamic race detection tool that "Traits::New" above
            // and CompleteLazyInstance(...) happens before "return instance()" below.
            // See the corresponding HAPPENS_BEFORE in CompleteLazyInstance(...).
            ANNOTATE_HAPPENS_AFTER(&private_instance_);
            return instance();
        }

        bool operator==(Type *p) {
            switch (private_instance_.load()) {
                case 0:
                    return p == NULL;
                case internal::kLazyInstanceStateCreating:
                    return static_cast<void *>(p) == private_buf_.void_data();
                default:
                    return p == instance();
            }
        }

        // Effectively private: member data is only public to allow the linker to
        // statically initialize it and to maintain a POD class. DO NOT USE FROM
        // OUTSIDE THIS CLASS.

        std::atomic<intptr_t> private_instance_;
        // Preallocated space for the Type instance.
        butil::AlignedMemory<sizeof(Type), ALIGNOF(Type)> private_buf_;

    private:
        Type *instance() {
            return reinterpret_cast<Type *>(private_instance_.load());
        }

        // Adapter function for use with AtExit.  This should be called single
        // threaded, so don't synchronize across threads.
        // Calling OnExit while the instance is in use by other threads is a mistake.
        static void OnExit(void *lazy_instance) {
            LazyInstance<Type, Traits> *me =
                    reinterpret_cast<LazyInstance<Type, Traits> *>(lazy_instance);
            Traits::Delete(me->instance());
            me->private_instance_.store(0);
        }
    };

}  // namespace butil

#endif  // BUTIL_LAZY_INSTANCE_H_
