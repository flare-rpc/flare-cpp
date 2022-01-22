// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef FLARE_BASE_STR_SPLIT_H_
#define FLARE_BASE_STR_SPLIT_H_


#include <array>
#include <initializer_list>
#include <iterator>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>
#include <string_view>
#include "flare/base/profile.h"
#include "flare/base/strings.h"



namespace flare::base {
    namespace string_internal {
        // This class is implicitly constructible from everything that std::string_view
        // is implicitly constructible from. If it's constructed from a temporary
        // string, the data is moved into a data member so its lifetime matches that of
        // the convertible_to_string_view instance.
        class convertible_to_string_view {
        public:
            convertible_to_string_view(const char *s)  // NOLINT(runtime/explicit)
                    : value_(s) {}

            convertible_to_string_view(char *s) : value_(s) {}  // NOLINT(runtime/explicit)
            convertible_to_string_view(std::string_view s)     // NOLINT(runtime/explicit)
                    : value_(s) {}

            convertible_to_string_view(const std::string &s)  // NOLINT(runtime/explicit)
                    : value_(s) {}

            // Matches rvalue strings and moves their data to a member.
            convertible_to_string_view(std::string &&s)  // NOLINT(runtime/explicit)
                    : copy_(std::move(s)), value_(copy_) {}

            convertible_to_string_view(const convertible_to_string_view &other)
                    : copy_(other.copy_),
                      value_(other.is_self_referential() ? copy_ : other.value_) {}

            convertible_to_string_view(convertible_to_string_view &&other) {
                steal_members(std::move(other));
            }

            convertible_to_string_view &operator=(convertible_to_string_view other) {
                steal_members(std::move(other));
                return *this;
            }

            std::string_view value() const { return value_; }

        private:
            // Returns true if ctsp's value refers to its internal copy_ member.
            bool is_self_referential() const { return value_.data() == copy_.data(); }

            void steal_members(convertible_to_string_view &&other) {
                if (other.is_self_referential()) {
                    copy_ = std::move(other.copy_);
                    value_ = copy_;
                    other.value_ = other.copy_;
                } else {
                    value_ = other.value_;
                }
            }

            // Holds the data moved from temporary std::string arguments. Declared first
            // so that 'value' can refer to 'copy_'.
            std::string copy_;
            std::string_view value_;
        };

        // has_mapped_type<T>::value is true iff there exists a type T::mapped_type.
        template<typename T, typename = void>
        struct has_mapped_type : std::false_type {
        };
        template<typename T>
        struct has_mapped_type<T, std::void_t < typename T::mapped_type>>
        : std::true_type {
        };

        // has_value_type<T>::value is true iff there exists a type T::value_type.
        template<typename T, typename = void>
        struct has_value_type : std::false_type {
        };

        template<typename T>
        struct has_value_type<T, std::void_t < typename T::value_type>> : std::true_type {
        };

        // has_const_iterator<T>::value is true iff there exists a type T::const_iterator.
        template<typename T, typename = void>
        struct has_const_iterator : std::false_type {
        };
        template<typename T>
        struct has_const_iterator<T, std::void_t < typename T::const_iterator>>
        : std::true_type {
        };

        // is_initializer_list<T>::value is true iff T is an std::initializer_list. More
        // details below in Splitter<> where this is used.
        std::false_type is_initializer_list_dispatch(...);  // default: No
        template<typename T>
        std::true_type is_initializer_list_dispatch(std::initializer_list <T> *);

        template<typename T>
        struct is_initializer_list
                : decltype(is_initializer_list_dispatch(static_cast<T *>(nullptr))) {
        };

        // A splitter_is_convertible_to<C>::type alias exists iff the specified condition
        // is true for type 'C'.
        //
        // Restricts conversion to container-like types (by testing for the presence of
        // a const_iterator member type) and also to disable conversion to an
        // std::initializer_list (which also has a const_iterator). Otherwise, code
        // compiled in C++11 will get an error due to ambiguous conversion paths (in
        // C++11 std::vector<T>::operator= is overloaded to take either a std::vector<T>
        // or an std::initializer_list<T>).

        template<typename C, bool has_value_type, bool has_mapped_type>
        struct splitter_is_convertible_to_impl : std::false_type {
        };

        template<typename C>
        struct splitter_is_convertible_to_impl<C, true, false>
                : std::is_constructible<typename C::value_type, std::string_view> {
        };

        template<typename C>
        struct splitter_is_convertible_to_impl<C, true, true>
                : std::conjunction<
                        std::is_constructible < typename C::key_type, std::string_view>,
                  std::is_constructible<typename C::mapped_type, std::string_view>> {
        };

        template<typename C>
        struct splitter_is_convertible_to
                : splitter_is_convertible_to_impl<
                        C,
                        !is_initializer_list<
                                typename std::remove_reference<C>::type>::value &&
                        has_value_type<C>::value && has_const_iterator<C>::value,
                        has_mapped_type<C>::value> {
        };

    }  // namespace string_internal

    // An iterator that enumerates the parts of a string from a Splitter. The text
    // to be split, the Delimiter, and the Predicate are all taken from the given
    // Splitter object. Iterators may only be compared if they refer to the same
    // Splitter instance.

    template<typename Splitter>
    class split_iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string_view;
        using difference_type = ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        enum State {
            kInitState, kLastState, kEndState
        };

        split_iterator(State state, const Splitter *splitter)
                : pos_(0),
                  state_(state),
                  splitter_(splitter),
                  delimiter_(splitter->delimiter()),
                  predicate_(splitter->predicate()) {
            // Hack to maintain backward compatibility. This one block makes it so an
            // empty std::string_view whose .data() happens to be nullptr behaves
            // *differently* from an otherwise empty std::string_view whose .data() is
            // not nullptr. This is an undesirable difference in general, but this
            // behavior is maintained to avoid breaking existing code that happens to
            // depend on this old behavior/bug. Perhaps it will be fixed one day. The
            // difference in behavior is as follows:
            //   Split(std::string_view(""), '-');  // {""}
            //   Split(std::string_view(), '-');    // {}
            if (splitter_->text().data() == nullptr) {
                state_ = kEndState;
                pos_ = splitter_->text().size();
                return;
            }

            if (state_ == kEndState) {
                pos_ = splitter_->text().size();
            } else {
                ++(*this);
            }
        }

        bool at_end() const { return state_ == kEndState; }

        reference operator*() const { return curr_; }

        pointer operator->() const { return &curr_; }

        operator bool() const { return !at_end(); }

        split_iterator &operator++() {
            do {
                if (state_ == kLastState) {
                    state_ = kEndState;
                    return *this;
                }
                const std::string_view text = splitter_->text();
                const std::string_view d = delimiter_.find(text, pos_);
                if (d.data() == text.data() + text.size()) state_ = kLastState;
                curr_ = text.substr(pos_, d.data() - (text.data() + pos_));
                pos_ += curr_.size() + d.size();
            } while (!predicate_(curr_));
            return *this;
        }

        split_iterator operator++(int) {
            split_iterator old(*this);
            ++(*this);
            return old;
        }

        friend bool operator==(const split_iterator &a, const split_iterator &b) {
            return a.state_ == b.state_ && a.pos_ == b.pos_;
        }

        friend bool operator!=(const split_iterator &a, const split_iterator &b) {
            return !(a == b);
        }

    private:
        size_t pos_;
        State state_;
        std::string_view curr_;
        const Splitter *splitter_;
        typename Splitter::DelimiterType delimiter_;
        typename Splitter::PredicateType predicate_;
    };
    // This class implements the range that is returned by flare:: string_split(). This
    // class has templated conversion operators that allow it to be implicitly
    // converted to a variety of types that the caller may have specified on the
    // left-hand side of an assignment.
    //
    // The main interface for interacting with this class is through its implicit
    // conversion operators. However, this class may also be used like a container
    // in that it has .begin() and .end() member functions. It may also be used
    // within a range-for loop.
    //
    // Output containers can be collections of any type that is constructible from
    // an std::string_view.
    //
    // An Predicate functor may be supplied. This predicate will be used to filter
    // the split strings: only strings for which the predicate returns true will be
    // kept. A Predicate object is any unary functor that takes an std::string_view
    // and returns bool.
    template<typename Delimiter, typename Predicate>
    class splitter {
    public:
        using DelimiterType = Delimiter;
        using PredicateType = Predicate;
        using const_iterator = split_iterator<splitter>;
        using value_type = typename std::iterator_traits<const_iterator>::value_type;

        splitter(string_internal::convertible_to_string_view input_text, Delimiter d, Predicate p)
                : text_(std::move(input_text)),
                  delimiter_(std::move(d)),
                  predicate_(std::move(p)) {}

        std::string_view text() const { return text_.value(); }

        const Delimiter &delimiter() const { return delimiter_; }

        const Predicate &predicate() const { return predicate_; }

        // Range functions that iterate the split substrings as std::string_view
        // objects. These methods enable a Splitter to be used in a range-based for
        // loop.
        const_iterator begin() const { return {const_iterator::kInitState, this}; }

        const_iterator end() const { return {const_iterator::kEndState, this}; }

        // An implicit conversion operator that is restricted to only those containers
        // that the splitter is convertible to.
        template<typename Container,
                typename = typename std::enable_if<
                        string_internal::splitter_is_convertible_to<Container>::value>::type>
        operator Container() const {  // NOLINT(runtime/explicit)
            return convert_to_container<Container, typename Container::value_type,
                    string_internal::has_mapped_type<Container>::value>()(*this);
        }

        // Returns a pair with its .first and .second members set to the first two
        // strings returned by the begin() iterator. Either/both of .first and .second
        // will be constructed with empty strings if the iterator doesn't have a
        // corresponding value.
        template<typename First, typename Second>
        operator std::pair<First, Second>() const {  // NOLINT(runtime/explicit)
            std::string_view first, second;
            auto it = begin();
            if (it != end()) {
                first = *it;
                if (++it != end()) {
                    second = *it;
                }
            }
            return {First(first), Second(second)};
        }

    private:
        // convert_to_container is a functor converting a Splitter to the requested
        // Container of ValueType. It is specialized below to optimize splitting to
        // certain combinations of Container and ValueType.
        //
        // This base template handles the generic case of storing the split results in
        // the requested non-map-like container and converting the split substrings to
        // the requested type.
        template<typename Container, typename ValueType, bool is_map = false>
        struct convert_to_container {
            Container operator()(const splitter &splitter) const {
                Container c;
                auto it = std::inserter(c, c.end());
                for (const auto &sp : splitter) {
                    *it++ = ValueType(sp);
                }
                return c;
            }
        };

        // Partial specialization for a std::vector<std::string_view>.
        //
        // Optimized for the common case of splitting to a
        // std::vector<std::string_view>. In this case we first split the results to
        // a small array of std::string_view on the stack, to reduce reallocations.
        template<typename A>
        struct convert_to_container<std::vector < std::string_view, A>,
        std::string_view, false> {
            std::vector <std::string_view, A> operator()(
                    const splitter &splitter) const {
                struct raw_view {
                    const char *data;
                    size_t size;

                    operator std::string_view() const {  // NOLINT(runtime/explicit)
                        return {data, size};
                    }
                };
                std::vector <std::string_view, A> v;
                std::array<raw_view, 16> ar;
                for (auto it = splitter.begin(); !it.at_end();) {
                    size_t index = 0;
                    do {
                        ar[index].data = it->data();
                        ar[index].size = it->size();
                        ++it;
                    } while (++index != ar.size() && !it.at_end());
                    v.insert(v.end(), ar.begin(), ar.begin() + index);
                }
                return v;
            }
        };

        // Partial specialization for a std::vector<std::string>.
        //
        // Optimized for the common case of splitting to a std::vector<std::string>.
        // In this case we first split the results to a std::vector<std::string_view>
        // so the returned std::vector<std::string> can have space reserved to avoid
        // std::string moves.
        template<typename A>
        struct convert_to_container<std::vector < std::string, A>, std::string, false> {
            std::vector <std::string, A> operator()(const splitter &splitter) const {
                const std::vector <std::string_view> v = splitter;
                return std::vector<std::string, A>(v.begin(), v.end());
            }
        };

        // Partial specialization for containers of pairs (e.g., maps).
        //
        // The algorithm is to insert a new pair into the map for each even-numbered
        // item, with the even-numbered item as the key with a default-constructed
        // value. Each odd-numbered item will then be assigned to the last pair's
        // value.
        template<typename Container, typename First, typename Second>
        struct convert_to_container<Container, std::pair<const First, Second>, true> {
            Container operator()(const splitter &splitter) const {
                Container m;
                typename Container::iterator it;
                bool insert = true;
                for (const auto &sp : splitter) {
                    if (insert) {
                        it = inserter<Container>::Insert(&m, First(sp), Second());
                    } else {
                        it->second = Second(sp);
                    }
                    insert = !insert;
                }
                return m;
            }

            // Inserts the key and value into the given map, returning an iterator to
            // the inserted item. Specialized for std::map and std::multimap to use
            // emplace() and adapt emplace()'s return value.
            template<typename Map>
            struct inserter {
                using M = Map;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->insert(std::make_pair(std::forward<Args>(args)...)).first;
                }
            };

            template<typename... Ts>
            struct inserter<std::map < Ts...>> {
                using M = std::map<Ts...>;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->emplace(std::make_pair(std::forward<Args>(args)...)).first;
                }
            };

            template<typename... Ts>
            struct inserter<std::multimap < Ts...>> {
                using M = std::multimap<Ts...>;

                template<typename... Args>
                static typename M::iterator Insert(M *m, Args &&... args) {
                    return m->emplace(std::make_pair(std::forward<Args>(args)...));
                }
            };
        };

        string_internal::convertible_to_string_view text_;
        Delimiter delimiter_;
        Predicate predicate_;
    };


    //------------------------------------------------------------------------------
    // Delimiters
    //------------------------------------------------------------------------------
    //
    // ` string_split()` uses delimiters to define the boundaries between elements in the
    // provided input. Several `Delimiter` types are defined below. If a string
    // (`const char*`, `std::string`, or `std::string_view`) is passed in place of
    // an explicit `Delimiter` object, ` string_split()` treats it the same way as if it
    // were passed a `by_string` delimiter.
    //
    // A `Delimiter` is an object with a `find()` function that knows how to find
    // the first occurrence of itself in a given `std::string_view`.
    //
    // The following `Delimiter` types are available for use within ` string_split()`:
    //
    //   - `by_string` (default for string arguments)
    //   - `by_char` (default for a char argument)
    //   - `by_any_char`
    //   - `by_length`
    //   - `max_splits`
    //
    // A Delimiter's `find()` member function will be passed an input `text` that is
    // to be split and a position (`pos`) to begin searching for the next delimiter
    // in `text`. The returned std::string_view should refer to the next occurrence
    // (after `pos`) of the represented delimiter; this returned std::string_view
    // represents the next location where the input `text` should be broken.
    //
    // The returned std::string_view may be zero-length if the Delimiter does not
    // represent a part of the string (e.g., a fixed-length delimiter). If no
    // delimiter is found in the input `text`, a zero-length std::string_view
    // referring to `text.end()` should be returned (e.g.,
    // `text.substr(text.size())`). It is important that the returned
    // std::string_view always be within the bounds of the input `text` given as an
    // argument--it must not refer to a string that is physically located outside of
    // the given string.
    //
    // The following example is a simple Delimiter object that is created with a
    // single char and will look for that char in the text passed to the `find()`
    // function:
    //
    //   struct SimpleDelimiter {
    //     const char c_;
    //     explicit SimpleDelimiter(char c) : c_(c) {}
    //     std::string_view find(std::string_view text, size_t pos) {
    //       auto found = text.find(c_, pos);
    //       if (found == std::string_view::npos)
    //         return text.substr(text.size());
    //
    //       return text.substr(found, 1);
    //     }
    //   };

    // by_string
    //
    // A sub-string delimiter. If ` string_split()` is passed a string in place of a
    // `Delimiter` object, the string will be implicitly converted into a
    // `by_string` delimiter.
    //
    // Example:
    //
    //   // Because a string literal is converted to an `flare::by_string`,
    //   // the following two splits are equivalent.
    //
    //   std::vector<std::string> v1 = flare:: string_split("a, b, c", ", ");
    //
    //   using flare::base::by_string;
    //   std::vector<std::string> v2 = flare::base:: string_split("a, b, c",
    //                                                by_string(", "));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    class by_string {
    public:
        explicit by_string(std::string_view sp);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const std::string _delimiter;
    };

    // by_char
    //
    // A single character delimiter. `by_char` is functionally equivalent to a
    // 1-char string within a `by_string` delimiter, but slightly more efficient.
    //
    // Example:
    //
    //   // Because a char literal is converted to a flare::base::by_char,
    //   // the following two splits are equivalent.
    //   std::vector<std::string> v1 = flare::base:: string_split("a,b,c", ',');
    //   using flare::base::by_char;
    //   std::vector<std::string> v2 = flare::base:: string_split("a,b,c", by_char(','));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // `by_char` is also the default delimiter if a single character is given
    // as the delimiter to ` string_split()`. For example, the following calls are
    // equivalent:
    //
    //   std::vector<std::string> v = flare::base:: string_split("a-b", '-');
    //
    //   using flare::base::by_char;
    //   std::vector<std::string> v = flare::base:: string_split("a-b", by_char('-'));
    //
    class by_char {
    public:
        explicit by_char(char c) : c_(c) {}

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        char c_;
    };

    // by_any_char
    //
    // A delimiter that will match any of the given byte-sized characters within
    // its provided string.
    //
    // Note: this delimiter works with single-byte string data, but does not work
    // with variable-width encodings, such as UTF-8.
    //
    // Example:
    //
    //   using flare::base::by_any_char;
    //   std::vector<std::string> v = flare::base:: string_split("a,b=c", by_any_char(",="));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // If `by_any_char` is given the empty string, it behaves exactly like
    // `by_string` and matches each individual character in the input string.
    //
    class by_any_char {
    public:
        explicit by_any_char(std::string_view sp);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const std::string _delimiters;
    };

    //  by_length
    //
    // A delimiter for splitting into equal-length strings. The length argument to
    // the constructor must be greater than 0.
    //
    // Note: this delimiter works with single-byte string data, but does not work
    // with variable-width encodings, such as UTF-8.
    //
    // Example:
    //
    //   using flare::base:: by_length;
    //   std::vector<std::string> v = flare::base:: string_split("123456789",  by_length(3));

    //   // v[0] == "123", v[1] == "456", v[2] == "789"
    //
    // Note that the string does not have to be a multiple of the fixed split
    // length. In such a case, the last substring will be shorter.
    //
    //   using flare::base:: by_length;
    //   std::vector<std::string> v = flare::base:: string_split("12345",  by_length(2));
    //
    //   // v[0] == "12", v[1] == "34", v[2] == "5"
    class by_length {
    public:
        explicit by_length(ptrdiff_t length);

        std::string_view find(std::string_view text, size_t pos) const;

    private:
        const ptrdiff_t _length;
    };

    namespace string_internal {

        // A traits-like metafunction for selecting the default Delimiter object type
        // for a particular Delimiter type. The base case simply exposes type Delimiter
        // itself as the delimiter's Type. However, there are specializations for
        // string-like objects that map them to the by_string delimiter object.
        // This allows functions like flare::base:: string_split() and flare::base:: max_splits() to accept
        // string-like objects (e.g., ',') as delimiter arguments but they will be
        // treated as if a by_string delimiter was given.
        template<typename Delimiter>
        struct select_delimiter {
            using type = Delimiter;
        };

        template<>
        struct select_delimiter<char> {
            using type = by_char;
        };
        template<>
        struct select_delimiter<char *> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<const char *> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<std::string_view> {
            using type = by_string;
        };
        template<>
        struct select_delimiter<std::string> {
            using type = by_string;
        };

    // Wraps another delimiter and sets a max number of matches for that delimiter.
        template<typename Delimiter>
        class max_splits_impl {
        public:
            max_splits_impl(Delimiter delimiter, int limit)
                    : _delimiter(delimiter), _limit(limit), _count(0) {}

            std::string_view find(std::string_view text, size_t pos) {
                if (_count++ == _limit) {
                    return std::string_view(text.data() + text.size(),
                                            0);  // No more matches.
                }
                return _delimiter.find(text, pos);
            }

        private:
            Delimiter _delimiter;
            const int _limit;
            int _count;
        };

    }  // namespace string_internal

    //  max_splits()
    //
    // A delimiter that limits the number of matches which can occur to the passed
    // `limit`. The last element in the returned collection will contain all
    // remaining unsplit pieces, which may contain instances of the delimiter.
    // The collection will contain at most `limit` + 1 elements.
    // Example:
    //
    //   using flare::base:: max_splits;
    //   std::vector<std::string> v = flare::base:: string_split("a,b,c",  max_splits(',', 1));
    //
    //   // v[0] == "a", v[1] == "b,c"
    template<typename Delimiter>
    FLARE_FORCE_INLINE string_internal::max_splits_impl<
            typename string_internal::select_delimiter<Delimiter>::type>
    max_splits(Delimiter delimiter, int limit) {
    typedef
    typename string_internal::select_delimiter<Delimiter>::type DelimiterType;
    return string_internal::max_splits_impl<DelimiterType>(
            DelimiterType(delimiter), limit);
    }

    //------------------------------------------------------------------------------
    // Predicates
    //------------------------------------------------------------------------------
    //
    // Predicates filter the results of a ` string_split()` by determining whether or not
    // a resultant element is included in the result set. A predicate may be passed
    // as an optional third argument to the ` string_split()` function.
    //
    // Predicates are unary functions (or functors) that take a single
    // `std::string_view` argument and return a bool indicating whether the
    // argument should be included (`true`) or excluded (`false`).
    //
    // Predicates are useful when filtering out empty substrings. By default, empty
    // substrings may be returned by ` string_split()`, which is similar to the way split
    // functions work in other programming languages.

    //  allow_empty()
    //
    // Always returns `true`, indicating that all strings--including empty
    // strings--should be included in the split output. This predicate is not
    // strictly needed because this is the default behavior of ` string_split()`;
    // however, it might be useful at some call sites to make the intent explicit.
    //
    // Example:
    //
    //  std::vector<std::string> v = flare::base:: string_split(" a , ,,b,", ',',  allow_empty());
    //
    //  // v[0] == " a ", v[1] == " ", v[2] == "", v[3] = "b", v[4] == ""
    struct allow_empty {
        bool operator()(std::string_view) const { return true; }
    };

    //  skip_empty()
    //
    // Returns `false` if the given `std::string_view` is empty, indicating that
    // ` string_split()` should omit the empty string.
    //
    // Example:
    //
    //   std::vector<std::string> v = flare::base:: string_split(",a,,b,", ',',  skip_empty());
    //
    //   // v[0] == "a", v[1] == "b"
    //
    // Note: ` skip_empty()` does not consider a string containing only whitespace
    // to be empty. To skip such whitespace as well, use the ` skip_whitespace()`
    // predicate.
    struct skip_empty {
        bool operator()(std::string_view sp) const { return !sp.empty(); }
    };

    //  skip_whitespace()
    //
    // Returns `false` if the given `std::string_view` is empty *or* contains only
    // whitespace, indicating that ` string_split()` should omit the string.
    //
    // Example:
    //
    //   std::vector<std::string> v = flare::base:: string_split(" a , ,,b,",
    //                                               ',',  skip_whitespace());
    //   // v[0] == " a ", v[1] == "b"
    //
    //   //  skip_empty() would return whitespace elements
    //   std::vector<std::string> v = flare::base:: string_split(" a , ,,b,", ',',  skip_empty());
    //   // v[0] == " a ", v[1] == " ", v[2] == "b"
    struct skip_whitespace {
        bool operator()(std::string_view sp) const {
            sp = flare::base::strip_ascii_whitespace(sp);
            return !sp.empty();
        }
    };

    //------------------------------------------------------------------------------
    //                                   string_split()
    //------------------------------------------------------------------------------

    //  string_split()
    //
    // Splits a given string based on the provided `Delimiter` object, returning the
    // elements within the type specified by the caller. Optionally, you may pass a
    // `Predicate` to ` string_split()` indicating whether to include or exclude the
    // resulting element within the final result set. (See the overviews for
    // Delimiters and Predicates above.)
    //
    // Example:
    //
    //   std::vector<std::string> v = flare::base:: string_split("a,b,c,d", ',');
    //   // v[0] == "a", v[1] == "b", v[2] == "c", v[3] == "d"
    //
    // You can also provide an explicit `Delimiter` object:
    //
    // Example:
    //
    //   using flare::base::by_any_char;
    //   std::vector<std::string> v = flare::base:: string_split("a,b=c", by_any_char(",="));
    //   // v[0] == "a", v[1] == "b", v[2] == "c"
    //
    // See above for more information on delimiters.
    //
    // By default, empty strings are included in the result set. You can optionally
    // include a third `Predicate` argument to apply a test for whether the
    // resultant element should be included in the result set:
    //
    // Example:
    //
    //   std::vector<std::string> v = flare::base:: string_split(" a , ,,b,",
    //                                               ',',  skip_whitespace());
    //   // v[0] == " a ", v[1] == "b"
    //
    // See above for more information on predicates.
    //
    //------------------------------------------------------------------------------
    //  string_split() Return Types
    //------------------------------------------------------------------------------
    //
    // The ` string_split()` function adapts the returned collection to the collection
    // specified by the caller (e.g. `std::vector` above). The returned collections
    // may contain `std::string`, `std::string_view` (in which case the original
    // string being split must ensure that it outlives the collection), or any
    // object that can be explicitly created from an `std::string_view`. This
    // behavior works for:
    //
    // 1) All standard STL containers including `std::vector`, `std::list`,
    //    `std::deque`, `std::set`,`std::multiset`, 'std::map`, and `std::multimap`
    // 2) `std::pair` (which is not actually a container). See below.
    //
    // Example:
    //
    //   // The results are returned as `std::string_view` objects. Note that we
    //   // have to ensure that the input string outlives any results.
    //   std::vector<std::string_view> v = flare::base:: string_split("a,b,c", ',');
    //
    //   // Stores results in a std::set<std::string>, which also performs
    //   // de-duplication and orders the elements in ascending order.
    //   std::set<std::string> a = flare::base:: string_split("b,a,c,a,b", ',');
    //   // v[0] == "a", v[1] == "b", v[2] = "c"
    //
    //   // ` string_split()` can be used within a range-based for loop, in which case
    //   // each element will be of type `std::string_view`.
    //   std::vector<std::string> v;
    //   for (const auto sv : flare::base:: string_split("a,b,c", ',')) {
    //     if (sv != "b") v.emplace_back(sv);
    //   }
    //   // v[0] == "a", v[1] == "c"
    //
    //   // Stores results in a map. The map implementation assumes that the input
    //   // is provided as a series of key/value pairs. For example, the 0th element
    //   // resulting from the split will be stored as a key to the 1st element. If
    //   // an odd number of elements are resolved, the last element is paired with
    //   // a default-constructed value (e.g., empty string).
    //   std::map<std::string, std::string> m = flare::base:: string_split("a,b,c", ',');
    //   // m["a"] == "b", m["c"] == ""     // last component value equals ""
    //
    // Splitting to `std::pair` is an interesting case because it can hold only two
    // elements and is not a collection type. When splitting to a `std::pair` the
    // first two split strings become the `std::pair` `.first` and `.second`
    // members, respectively. The remaining split substrings are discarded. If there
    // are less than two split substrings, the empty string is used for the
    // corresponding
    // `std::pair` member.
    //
    // Example:
    //
    //   // Stores first two split strings as the members in a std::pair.
    //   std::pair<std::string, std::string> p = flare::base:: string_split("a,b,c", ',');
    //   // p.first == "a", p.second == "b"       // "c" is omitted.
    //
    // The ` string_split()` function can be used multiple times to perform more
    // complicated splitting logic, such as intelligently parsing key-value pairs.
    //
    // Example:
    //
    //   // The input string "a=b=c,d=e,f=,g" becomes
    //   // { "a" => "b=c", "d" => "e", "f" => "", "g" => "" }
    //   std::map<std::string, std::string> m;
    //   for (std::string_view sp : flare::base:: string_split("a=b=c,d=e,f=,g", ',')) {
    //     m.insert(flare::base:: string_split(sp, flare::base:: max_splits('=', 1)));
    //   }
    //   EXPECT_EQ("b=c", m.find("a")->second);
    //   EXPECT_EQ("e", m.find("d")->second);
    //   EXPECT_EQ("", m.find("f")->second);
    //   EXPECT_EQ("", m.find("g")->second);
    //
    // WARNING: Due to a legacy bug that is maintained for backward compatibility,
    // splitting the following empty string_views produces different results:
    //
    //   flare::base:: string_split(std::string_view(""), '-');  // {""}
    //   flare::base:: string_split(std::string_view(), '-');    // {}, but should be {""}
    //
    // Try not to depend on this distinction because the bug may one day be fixed.
    template<typename Delimiter> splitter<typename string_internal::select_delimiter<Delimiter>::type, allow_empty>
    string_split(string_internal::convertible_to_string_view text, Delimiter d) {
        using DelimiterType =
        typename string_internal::select_delimiter<Delimiter>::type;
        return splitter<DelimiterType, allow_empty>(
                std::move(text), DelimiterType(d), allow_empty());
    }

    template<typename Delimiter, typename Predicate>
    splitter<typename string_internal::select_delimiter<Delimiter>::type, Predicate>
    string_split(string_internal::convertible_to_string_view text, Delimiter d,
                 Predicate p) {
        using DelimiterType = typename string_internal::select_delimiter<Delimiter>::type;
        return splitter<DelimiterType, Predicate>(
                std::move(text), DelimiterType(d), std::move(p));
    }



}  // namespace flare::base

#endif  // FLARE_BASE_STR_SPLIT_H_
