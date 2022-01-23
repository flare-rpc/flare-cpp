//
// Created by liyinbin on 2022/1/22.
//

#ifndef FLARE_BASE_PROFILE_ATTRIBUTE_H_
#define FLARE_BASE_PROFILE_ATTRIBUTE_H_

#include "flare/base/profile/compiler.h"

// Annotate a function indicating it should not be inlined.
// Use like:
//   NOINLINE void DoStuff() { ... }
#ifndef FLARE_NO_INLINE
#if defined(FLARE_COMPILER_GNUC)
#define FLARE_NO_INLINE __attribute__((noinline))
#elif defined(FLARE_COMPILER_MSVC)
#define FLARE_NO_INLINE __declspec(noinline)
#else
#define FLARE_NO_INLINE
#endif
#endif  // FLARE_NO_INLINE

#ifndef FLARE_FORCE_INLINE
#if defined(FLARE_COMPILER_MSVC)
#define FLARE_FORCE_INLINE    __forceinline
#else
#define FLARE_FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif  // FLARE_FORCE_INLINE

#ifndef FLARE_ALLOW_UNUSED
#if defined(FLARE_COMPILER_GNUC)
#define FLARE_ALLOW_UNUSED __attribute__((unused))
#else
#define FLARE_ALLOW_UNUSED
#endif
#endif  // FLARE_ALLOW_UNUSED

#ifndef FLARE_HAVE_ATTRIBUTE
#ifdef __has_attribute
#define FLARE_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define FLARE_HAVE_ATTRIBUTE(x) 0
#endif
#endif  // FLARE_HAVE_ATTRIBUTE

#ifndef FLARE_MUST_USE_RESULT
#if FLARE_HAVE_ATTRIBUTE(nodiscard)
#define FLARE_MUST_USE_RESULT [[nodiscard]]
#elif defined(__clang__) && FLARE_HAVE_ATTRIBUTE(warn_unused_result)
#define FLARE_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define FLARE_MUST_USE_RESULT
#endif
#endif  // FLARE_MUST_USE_RESULT

#define FLARE_ARRAY_SIZE(array) \
  (sizeof(::flare::base::base_internal::ArraySizeHelper(array)))

namespace flare::base::base_internal {
    template<typename T, size_t N>
    auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace flare::base::base_internal

// FLARE_DEPRECATED void dont_call_me_anymore(int arg);
// ...
// warning: 'void dont_call_me_anymore(int)' is deprecated
#if defined(FLARE_COMPILER_GNUC) ||  defined(FLARE_COMPILER_CLANG)
# define FLARE_DEPRECATED __attribute__((deprecated))
#elif defined(FLARE_COMPILER_MSVC)
# define FLARE_DEPRECATED __declspec(deprecated)
#else
# define FLARE_DEPRECATED
#endif

#endif // FLARE_BASE_PROFILE_ATTRIBUTE_H_
