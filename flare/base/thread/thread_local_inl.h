

#ifndef FLARE_BASE_THREAD_THREAD_LOCAL_INL_H_
#define FLARE_BASE_THREAD_THREAD_LOCAL_INL_H_

namespace flare::base {

    namespace detail {

        template<typename T>
        class ThreadLocalHelper {
        public:
            inline static T *get() {
                if (__builtin_expect(value != nullptr, 1)) {
                    return value;
                }
                value = new(std::nothrow) T;
                if (value != nullptr) {
                    flare::base::thread_atexit(delete_object<T>, value);
                }
                return value;
            }

            static FLARE_THREAD_LOCAL T*value;
        };

        template<typename T> FLARE_THREAD_LOCAL T *ThreadLocalHelper<T>::value = nullptr;

    }  // namespace detail

    template<typename T>
    inline T *get_thread_local() {
        return detail::ThreadLocalHelper<T>::get();
    }

}  // namespace flare::base

#endif  // FLARE_BASE_THREAD_THREAD_LOCAL_INL_H_
