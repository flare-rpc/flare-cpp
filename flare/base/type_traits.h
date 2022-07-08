
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_TYPE_TRAITS_H_
#define FLARE_BASE_TYPE_TRAITS_H_

#include <cstddef>  // For size_t.
#include <type_traits>

namespace flare {

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


    template<typename T>
    struct remove_cvref {
        typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
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


}  // namespace flare


#endif  // FLARE_BASE_TYPE_TRAITS_H_
