// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLARE_BASE_TYPE_TRAITS_H_
#define FLARE_BASE_TYPE_TRAITS_H_

#include <cstddef>  // For size_t.
#include <type_traits>

namespace flare::base {

    template<typename T>
    struct remove_volatile;
    template<typename T>
    struct remove_reference;
    template<typename T>
    struct remove_const_reference;
    template<typename T>
    struct add_volatile;
    template<typename T>
    struct add_cv;
    template<typename T>
    struct add_reference;
    template<typename T>
    struct add_const_reference;
    template<typename T>
    struct remove_pointer;
    template<typename T>
    struct add_cr_non_integral;


    namespace internal {

// Types YesType and NoType are guaranteed such that sizeof(YesType) <
// sizeof(NoType).
        typedef char YesType;

        struct NoType {
            YesType dummy[2];
        };

        // This class is an implementation detail for is_convertible, and you
        // don't need to know how it works to use is_convertible. For those
        // who care: we declare two different functions, one whose argument is
        // of type To and one with a variadic argument list. We give them
        // return types of different size, so we can use sizeof to trick the
        // compiler into telling us which function it would have chosen if we
        // had called it with an argument of type From.  See Alexandrescu's
        // _Modern C++ Design_ for more details on this sort of trick.

        struct ConvertHelper {
            template<typename To>
            static YesType Test(To);

            template<typename To>
            static NoType Test(...);

            template<typename From>
            static From &Create();
        };

// Used to determine if a type is a struct/union/class. Inspired by Boost's
// is_class type_trait implementation.
        struct IsClassHelper {
            template<typename C>
            static YesType Test(void(C::*)(void));

            template<typename C>
            static NoType Test(...);
        };

// For implementing is_empty
#if defined(FLARE_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable:4624) // destructor could not be generated
#endif

        template<typename T>
        struct EmptyHelper1 : public T {
            EmptyHelper1();  // hh compiler bug workaround
            int i[256];
        private:
            // suppress compiler warnings:
            EmptyHelper1(const EmptyHelper1 &);

            EmptyHelper1 &operator=(const EmptyHelper1 &);
        };

#if defined(FLARE_COMPILER_MSVC)
#pragma warning(pop)
#endif

        struct EmptyHelper2 {
            int i[256];
        };

    }  // namespace internal



    // True if T is an empty class/struct
    // NOTE: not work for union
    template<typename T>
    struct is_empty : std::integral_constant<bool, std::is_class<T>::value &&
                                                   sizeof(internal::EmptyHelper1<T>) ==
                                                   sizeof(internal::EmptyHelper2)> {
    };


    template<typename T>
    struct remove_volatile {
        typedef T type;
    };
    template<typename T>
    struct remove_volatile<T volatile> {
        typedef T type;
    };
    template<typename T>
    struct remove_cv {
        typedef typename std::remove_const<typename remove_volatile<T>::type>::type type;
    };

// Specified by TR1 [4.7.2] Reference modifications.
    template<typename T>
    struct remove_reference {
        typedef T type;
    };
    template<typename T>
    struct remove_reference<T &> {
        typedef T type;
    };

    template<typename T>
    struct add_reference {
        typedef T &type;
    };
    template<typename T>
    struct add_reference<T &> {
        typedef T &type;
    };
// Specializations for void which can't be referenced.
    template<>
    struct add_reference<void> {
        typedef void type;
    };
    template<>
    struct add_reference<void const> {
        typedef void const type;
    };
    template<>
    struct add_reference<void volatile> {
        typedef void volatile type;
    };
    template<>
    struct add_reference<void const volatile> {
        typedef void const volatile type;
    };

    // Shortcut for adding/removing const&
    template<typename T>
    struct add_const_reference {
        typedef typename add_reference<typename std::add_const<T>::type>::type type;
    };
    template<typename T>
    struct remove_const_reference {
        typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
    };

    // Add const& for non-integral types.
    // add_cr_non_integral<int>::type      -> int
    // add_cr_non_integral<FooClass>::type -> const FooClass&
    template<typename T>
    struct add_cr_non_integral {
        typedef typename std::conditional<std::is_integral<T>::value, T,
                typename add_reference<typename std::add_const<T>::type>::type>::type type;
    };


}  // namespace flare::base

#endif  // FLARE_BASE_TYPE_TRAITS_H_
