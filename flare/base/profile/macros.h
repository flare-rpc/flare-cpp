//
// Created by liyinbin on 2022/1/23.
//

#ifndef FLARE_BASE_PROFILE_MACROS_H_
#define FLARE_BASE_PROFILE_MACROS_H_

// Concatenate numbers in c/c++ macros.
#ifndef FLARE_CONCAT
# define FLARE_CONCAT(a, b) FLARE_CONCAT_HELPER(a, b)
# define FLARE_CONCAT_HELPER(a, b) a##b
#endif

#define FLARE_DELETE_FUNCTION(decl) decl = delete

#define FLARE_DISALLOW_COPY_AND_ASSIGN(TypeName)                      \
    FLARE_DELETE_FUNCTION(TypeName(const TypeName&));            \
    FLARE_DELETE_FUNCTION(void operator=(const TypeName&))



// DEFINE_SMALL_ARRAY(MyType, my_array, size, 64);
//   my_array is typed `MyType*' and as long as `size'. If `size' is not
//   greater than 64, the array is allocated on stack.
//
// NOTE: NEVER use ARRAY_SIZE(my_array) which is always 1.

#if defined(__cplusplus)
namespace flare::base {
    namespace internal {
        template <typename T> struct ArrayDeleter {
            ArrayDeleter() : arr(0) {}
            ~ArrayDeleter() { delete[] arr; }
            T* arr;
        };
    }}

// Many versions of clang does not support variable-length array with non-pod
// types, have to implement the macro differently.
#if !defined(__clang__)
# define DEFINE_SMALL_ARRAY(Tp, name, size, maxsize)                    \
    Tp* name = 0;                                                       \
    const unsigned name##_size = (size);                                \
    const unsigned name##_stack_array_size = (name##_size <= (maxsize) ? name##_size : 0); \
    Tp name##_stack_array[name##_stack_array_size];                     \
    ::flare::base::internal::ArrayDeleter<Tp> name##_array_deleter;            \
    if (name##_stack_array_size) {                                      \
        name = name##_stack_array;                                      \
    } else {                                                            \
        name = new (::std::nothrow) Tp[name##_size];                    \
        name##_array_deleter.arr = name;                                \
    }
#else
// This implementation works for GCC as well, however it needs extra 16 bytes
// for ArrayCtorDtor.
namespace flare::base {
    namespace internal {
        template <typename T> struct ArrayCtorDtor {
            ArrayCtorDtor(void* arr, unsigned size) : _arr((T*)arr), _size(size) {
                for (unsigned i = 0; i < size; ++i) { new (_arr + i) T; }
            }
            ~ArrayCtorDtor() {
                for (unsigned i = 0; i < _size; ++i) { _arr[i].~T(); }
            }
        private:
            T* _arr;
            unsigned _size;
        };
    }}
# define DEFINE_SMALL_ARRAY(Tp, name, size, maxsize)                    \
    Tp* name = 0;                                                       \
    const unsigned name##_size = (size);                                \
    const unsigned name##_stack_array_size = (name##_size <= (maxsize) ? name##_size : 0); \
    char name##_stack_array[sizeof(Tp) * name##_stack_array_size];      \
    ::flare::base::internal::ArrayDeleter<char> name##_array_deleter;          \
    if (name##_stack_array_size) {                                      \
        name = (Tp*)name##_stack_array;                                 \
    } else {                                                            \
        name = (Tp*)new (::std::nothrow) char[sizeof(Tp) * name##_size];\
        name##_array_deleter.arr = (char*)name;                         \
    }                                                                   \
    const ::flare::base::internal::ArrayCtorDtor<Tp> name##_array_ctor_dtor(name, name##_size);
#endif // !defined(__clang__)
#endif // defined(__cplusplus)


// Convert symbol to string
#ifndef FLARE_SYMBOLSTR
# define FLARE_SYMBOLSTR(a) FLARE_SYMBOLSTR_HELPER(a)
# define FLARE_SYMBOLSTR_HELPER(a) #a
#endif

// ptr:     the pointer to the member.
// type:    the type of the container struct this is embedded in.
// member:  the name of the member within the struct.
#ifndef FLARE_CONTAINER_OF
# define FLARE_CONTAINER_OF(ptr, type, member) ({                             \
            const decltype( ((type *)0)->member ) *__mptr = (ptr);  \
            (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

namespace flare::base {
    template<typename T>
    inline void ignore_result(const T&) {
    }
} // namespace flare::base

#endif  // FLARE_BASE_PROFILE_MACROS_H_
