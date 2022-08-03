
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_INLINE_VECTOR_H_
#define FLARE_CONTAINER_INLINE_VECTOR_H_

#include "flare/memory/allocator.h"
#include <algorithm>  // std::max
#include <cstddef>    // size_t
#include <utility>    // std::move

namespace flare {

    ////////////////////////////////////////////////////////////////////////////////
    // inline_vector<T, BASE_CAPACITY>
    ////////////////////////////////////////////////////////////////////////////////

    // inline_vector is a container of contiguously stored elements.
    // Unlike std::vector, flare::inline_vector keeps the first
    // BASE_CAPACITY elements internally, which will avoid dynamic heap
    // allocations. Once the inline_vector exceeds BASE_CAPACITY elements, inline_vector will
    // allocate storage from the heap.
    template<typename T, int BASE_CAPACITY>
    class inline_vector {
    public:
        inline inline_vector(allocator *allocator = allocator::Default);

        template<int BASE_CAPACITY_2>
        inline inline_vector(const inline_vector<T, BASE_CAPACITY_2> &other,
                      allocator *allocator = allocator::Default);

        template<int BASE_CAPACITY_2>
        inline inline_vector(inline_vector<T, BASE_CAPACITY_2> &&other,
                      allocator *allocator = allocator::Default);

        inline ~inline_vector();

        inline inline_vector &operator=(const inline_vector &);

        template<int BASE_CAPACITY_2>
        inline inline_vector<T, BASE_CAPACITY> &operator=(
                const inline_vector<T, BASE_CAPACITY_2> &);

        template<int BASE_CAPACITY_2>
        inline inline_vector<T, BASE_CAPACITY> &operator=(
                inline_vector<T, BASE_CAPACITY_2> &&);

        inline void push_back(const T &el);

        inline void emplace_back(T &&el);

        inline void pop_back();

        inline T &front();

        inline T &back();

        inline const T &front() const;

        inline const T &back() const;

        inline T *begin();

        inline T *end();

        inline const T *begin() const;

        inline const T *end() const;

        inline T &operator[](size_t i);

        inline const T &operator[](size_t i) const;

        inline size_t size() const;

        inline size_t cap() const;

        inline void resize(size_t n);

        inline void reserve(size_t n);

        inline T *data();

        inline const T *data() const;

    private:
        using TStorage = typename flare::aligned_storage<sizeof(T), alignof(T)>::type;

        inline_vector(const inline_vector &) = delete;

        inline void free();

        allocator *const _allocator;

        size_t _count = 0;
        size_t _capacity = BASE_CAPACITY;
        TStorage _buffer[BASE_CAPACITY];
        TStorage *_elements = _buffer;
        allocation _allocation;
    };

    template<typename T, int BASE_CAPACITY>
    inline_vector<T, BASE_CAPACITY>::inline_vector(
            flare::allocator *allocator /* = allocator::Default */)
            : _allocator(allocator) {}

    template<typename T, int BASE_CAPACITY>
    template<int BASE_CAPACITY_2>
    inline_vector<T, BASE_CAPACITY>::inline_vector(
            const inline_vector<T, BASE_CAPACITY_2> &other, flare::allocator *alloc /* = allocator::Default */)
            : _allocator(alloc) {
        *this = other;
    }

    template<typename T, int BASE_CAPACITY>
    template<int BASE_CAPACITY_2>
    inline_vector<T, BASE_CAPACITY>::inline_vector(
            inline_vector<T, BASE_CAPACITY_2> &&other,
            flare::allocator *allocator /* = allocator::Default */)
            : _allocator(allocator) {
        *this = std::move(other);
    }

    template<typename T, int BASE_CAPACITY>
    inline_vector<T, BASE_CAPACITY>::~inline_vector() {
        free();
    }

    template<typename T, int BASE_CAPACITY>
    inline_vector<T, BASE_CAPACITY> &inline_vector<T, BASE_CAPACITY>::operator=(
            const inline_vector &other) {
        free();
        reserve(other.size());
        _count = other.size();
        for (size_t i = 0; i < _count; i++) {
            new(&reinterpret_cast<T *>(_elements)[i]) T(other[i]);
        }
        return *this;
    }

    template<typename T, int BASE_CAPACITY>
    template<int BASE_CAPACITY_2>
    inline_vector<T, BASE_CAPACITY> &inline_vector<T, BASE_CAPACITY>::operator=(
            const inline_vector<T, BASE_CAPACITY_2> &other) {
        free();
        reserve(other.size());
        _count = other.size();
        for (size_t i = 0; i < _count; i++) {
            new(&reinterpret_cast<T *>(_elements)[i]) T(other[i]);
        }
        return *this;
    }

    template<typename T, int BASE_CAPACITY>
    template<int BASE_CAPACITY_2>
    inline_vector<T, BASE_CAPACITY> &inline_vector<T, BASE_CAPACITY>::operator=(
            inline_vector<T, BASE_CAPACITY_2> &&other) {
        free();
        reserve(other.size());
        _count = other.size();
        for (size_t i = 0; i < _count; i++) {
            new(&reinterpret_cast<T *>(_elements)[i]) T(std::move(other[i]));
        }
        other.resize(0);
        return *this;
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::push_back(const T &el) {
        reserve(_count + 1);
        new(&reinterpret_cast<T *>(_elements)[_count]) T(el);
        _count++;
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::emplace_back(T &&el) {
        reserve(_count + 1);
        new(&reinterpret_cast<T *>(_elements)[_count]) T(std::move(el));
        _count++;
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::pop_back() {
        FLARE_CHECK(_count > 0) << "pop_back() called on empty inline_vector";
        _count--;
        reinterpret_cast<T *>(_elements)[_count].~T();
    }

    template<typename T, int BASE_CAPACITY>
    T &inline_vector<T, BASE_CAPACITY>::front() {
        FLARE_CHECK(_count > 0) << "front() called on empty inline_vector";
        return reinterpret_cast<T *>(_elements)[0];
    }

    template<typename T, int BASE_CAPACITY>
    T &inline_vector<T, BASE_CAPACITY>::back() {
        FLARE_CHECK(_count > 0) << "back() called on empty inline_vector";
        return reinterpret_cast<T *>(_elements)[_count - 1];
    }

    template<typename T, int BASE_CAPACITY>
    const T &inline_vector<T, BASE_CAPACITY>::front() const {
        FLARE_CHECK(_count > 0) << "front() called on empty inline_vector";
        return reinterpret_cast<T *>(_elements)[0];
    }

    template<typename T, int BASE_CAPACITY>
    const T &inline_vector<T, BASE_CAPACITY>::back() const {
        FLARE_CHECK(_count > 0) << "back() called on empty inline_vector";
        return reinterpret_cast<T *>(_elements)[_count - 1];
    }

    template<typename T, int BASE_CAPACITY>
    T *inline_vector<T, BASE_CAPACITY>::begin() {
        return reinterpret_cast<T *>(_elements);
    }

    template<typename T, int BASE_CAPACITY>
    T *inline_vector<T, BASE_CAPACITY>::end() {
        return reinterpret_cast<T *>(_elements) + _count;
    }

    template<typename T, int BASE_CAPACITY>
    const T *inline_vector<T, BASE_CAPACITY>::begin() const {
        return reinterpret_cast<T *>(_elements);
    }

    template<typename T, int BASE_CAPACITY>
    const T *inline_vector<T, BASE_CAPACITY>::end() const {
        return reinterpret_cast<T *>(_elements) + _count;
    }

    template<typename T, int BASE_CAPACITY>
    T &inline_vector<T, BASE_CAPACITY>::operator[](size_t i) {
        FLARE_CHECK(i < _count);
        return reinterpret_cast<T *>(_elements)[i];
    }

    template<typename T, int BASE_CAPACITY>
    const T &inline_vector<T, BASE_CAPACITY>::operator[](size_t i) const {
        FLARE_CHECK(i < _count);
        return reinterpret_cast<T *>(_elements)[i];
    }

    template<typename T, int BASE_CAPACITY>
    size_t inline_vector<T, BASE_CAPACITY>::size() const {
        return _count;
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::resize(size_t n) {
        reserve(n);
        while (_count < n) {
            new(&reinterpret_cast<T *>(_elements)[_count++]) T();
        }
        while (n < _count) {
            reinterpret_cast<T *>(_elements)[--_count].~T();
        }
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::reserve(size_t n) {
        if (n > _capacity) {
            _capacity = std::max<size_t>(n * 2, 8);

            allocation::required_info request;
            request.size = sizeof(T) * _capacity;
            request.alignment = alignof(T);
            request.usage = allocation::Usage::Vector;

            auto a = _allocator->allocate(request);
            auto grown = reinterpret_cast<TStorage *>(a.ptr);
            for (size_t i = 0; i < _count; i++) {
                new(&reinterpret_cast<T *>(grown)[i])
                        T(std::move(reinterpret_cast<T *>(_elements)[i]));
            }
            free();
            _elements = grown;
            _allocation = a;
        }
    }

    template<typename T, int BASE_CAPACITY>
    T *inline_vector<T, BASE_CAPACITY>::data() {
        return _elements;
    }

    template<typename T, int BASE_CAPACITY>
    const T *inline_vector<T, BASE_CAPACITY>::data() const {
        return _elements;
    }

    template<typename T, int BASE_CAPACITY>
    void inline_vector<T, BASE_CAPACITY>::free() {
        for (size_t i = 0; i < _count; i++) {
            reinterpret_cast<T *>(_elements)[i].~T();
        }

        if (_allocation.ptr != nullptr) {
            _allocator->free(_allocation);
            _allocation = {};
            _elements = nullptr;
        }
    }

    
}  // namespace flare

#endif  // FLARE_CONTAINER_INLINE_VECTOR_H_
