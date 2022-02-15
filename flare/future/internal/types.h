//
// Created by liyinbin on 2022/2/15.
//

#ifndef FLARE_FUTURE_INTERNAL_TYPES_H_
#define FLARE_FUTURE_INTERNAL_TYPES_H_

#include <type_traits>

namespace flare::future_internal {

    template<class... Ts>
    struct empty_type {
    };

    // Get type at the specified location.
    template<class T, std::size_t I>
    struct types_at;
    template<class T, class... Ts>
    struct types_at<empty_type<T, Ts...>, 0> {
        using type = T;
    };  // Recursion boundary.
    template<std::size_t I, class T, class... Ts>
    struct types_at<empty_type<T, Ts...>, I> : types_at<empty_type<Ts...>, I - 1> {
    };

    template<class T, std::size_t I>
    using types_at_t = typename types_at<T, I>::type;
    static_assert(std::is_same_v<types_at_t<empty_type<int, char, void>, 1>, char>);

// Analogous to `std::tuple_cat`.
    template<class... Ts>
    struct types_cat;
    template<>
    struct types_cat<> {
        using type = empty_type<>;
    };  // Special case.
    template<class... Ts>
    struct types_cat<empty_type<Ts...>> {
        using type = empty_type<Ts...>;
    };  // Recursion boundary.
    template<class... Ts, class... Us, class... Vs>
    struct types_cat<empty_type<Ts...>, empty_type<Us...>, Vs...>
            : types_cat<empty_type<Ts..., Us...>, Vs...> {
    };

    template<class... Ts>
    using types_cat_t = typename types_cat<Ts...>::type;
    static_assert(std::is_same_v<types_cat_t<empty_type<int, double>, empty_type<void *>>,
            empty_type<int, double, void *>>);
    static_assert(
            std::is_same_v<types_cat_t<empty_type<int, double>, empty_type<void *>, empty_type<>>,
                    empty_type<int, double, void *>>);
    static_assert(std::is_same_v<
            types_cat_t<empty_type<int, double>, empty_type<void *>, empty_type<unsigned>>,
            empty_type<int, double, void *, unsigned>>);

// Check to see if a given type is listed in the types.
    template<class T, class U>
    struct types_contains;
    template<class U>
    struct types_contains<empty_type<>, U> : std::false_type {
    };
    template<class T, class U>
    struct types_contains<empty_type<T>, U> : std::is_same<T, U> {
    };
    template<class T, class U, class... Ts>
    struct types_contains<empty_type<T, Ts...>, U>
            : std::conditional_t<std::is_same_v<T, U>, std::true_type,
                    types_contains<empty_type<Ts...>, U>> {
    };

    template<class T, class U>
    constexpr auto types_contains_v = types_contains<T, U>::value;
    static_assert(types_contains_v<empty_type<int, char>, char>);
    static_assert(!types_contains_v<empty_type<int, char>, char *>);

// Erase all occurrance of a given type from `empty_type<...>`.
    template<class T, class U>
    struct types_erase;
    template<class U>
    struct types_erase<empty_type<>, U> {
        using type = empty_type<>;
    };  // Recursion boundary.
    template<class T, class U, class... Ts>
    struct types_erase<empty_type<T, Ts...>, U>
            : types_cat<std::conditional_t<!std::is_same_v<T, U>, empty_type<T>, empty_type<>>,
                    typename types_erase<empty_type<Ts...>, U>::type> {
    };

    template<class T, class U>
    using types_erase_t = typename types_erase<T, U>::type;
    static_assert(std::is_same_v<types_erase_t<empty_type<int, void, char>, void>,
            empty_type<int, char>>);
    static_assert(std::is_same_v<types_erase_t<empty_type<>, void>, empty_type<>>);
    static_assert(
            std::is_same_v<types_erase_t<empty_type<int, char>, void>, empty_type<int, char>>);

}  // namespace flare::future_internal
#endif  // FLARE_FUTURE_INTERNAL_TYPES_H_
