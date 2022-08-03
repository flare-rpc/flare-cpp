//
// Created by liyinbin on 2022/8/2.
//

#ifndef FLARE_CONTAINER_LIST_H_
#define FLARE_CONTAINER_LIST_H_

#include "flare/memory/allocator.h"

#include <algorithm>  // std::max
#include <cstddef>    // size_t
#include <utility>    // std::move

namespace flare {

    ////////////////////////////////////////////////////////////////////////////////
    // list<T, BASE_CAPACITY>
    ////////////////////////////////////////////////////////////////////////////////

    // list is a minimal std::list like container that supports constant time
    // insertion and removal of elements.
    // list keeps hold of allocations (it only releases allocations on destruction),
    // to avoid repeated heap allocations and frees when frequently inserting and
    // removing elements.
    template<typename T>
    class list {
        struct Entry {
            T data;
            Entry *next;
            Entry *prev;
        };

    public:
        class iterator {
        public:
            inline iterator(Entry *);

            inline T *operator->();

            inline T &operator*();

            inline iterator &operator++();

            inline bool operator==(const iterator &) const;

            inline bool operator!=(const iterator &) const;

        private:
            friend list;
            Entry *entry;
        };

        inline list(allocator *allocator = allocator::Default);

        inline ~list();

        inline iterator begin();

        inline iterator end();

        inline size_t size() const;

        template<typename... Args>
        inline iterator emplace_front(Args &&... args);

        inline void erase(iterator);

    private:

        // copy / move is currently unsupported.
        list(const list &) = delete;

        list(list &&) = delete;

        list &operator=(const list &) = delete;

        list &operator=(list &&) = delete;

        struct AllocationChain {
            allocation _allocation;
            AllocationChain *next;
        };

        inline void grow(size_t count);

        static inline void unlink(Entry *entry, Entry *&list);

        static inline void link(Entry *entry, Entry *&list);

        allocator *const alloc;
        size_t size_ = 0;
        size_t capacity = 0;
        AllocationChain *allocations = nullptr;
        Entry *free = nullptr;
        Entry *head = nullptr;
    };

    template<typename T>
    list<T>::iterator::iterator(Entry *entry) : entry(entry) {}

    template<typename T>
    T *list<T>::iterator::operator->() {
        return &entry->data;
    }

    template<typename T>
    T &list<T>::iterator::operator*() {
        return entry->data;
    }

    template<typename T>
    typename list<T>::iterator &list<T>::iterator::operator++() {
        entry = entry->next;
        return *this;
    }

    template<typename T>
    bool list<T>::iterator::operator==(const iterator &rhs) const {
        return entry == rhs.entry;
    }

    template<typename T>
    bool list<T>::iterator::operator!=(const iterator &rhs) const {
        return entry != rhs.entry;
    }

    template<typename T>
    list<T>::list(allocator *allocator /* = allocator::Default */)
            : alloc(allocator) {}

    template<typename T>
    list<T>::~list() {
        for (auto el = head; el != nullptr; el = el->next) {
            el->data.~T();
        }

        auto curr = allocations;
        while (curr != nullptr) {
            auto next = curr->next;
            alloc->free(curr->_allocation);
            curr = next;
        }
    }

    template<typename T>
    typename list<T>::iterator list<T>::begin() {
        return {head};
    }

    template<typename T>
    typename list<T>::iterator list<T>::end() {
        return {nullptr};
    }

    template<typename T>
    size_t list<T>::size() const {
        return size_;
    }

    template<typename T>
    template<typename... Args>
    typename list<T>::iterator list<T>::emplace_front(Args &&... args) {
        if (free == nullptr) {
            grow(std::max<size_t>(capacity, 8));
        }

        auto entry = free;

        unlink(entry, free);
        link(entry, head);

        new(&entry->data) T(std::forward<T>(args)...);
        size_++;

        return entry;
    }

    template<typename T>
    void list<T>::erase(iterator it) {
        auto entry = it.entry;
        unlink(entry, head);
        link(entry, free);

        entry->data.~T();
        size_--;
    }

    template<typename T>
    void list<T>::grow(size_t count) {
        auto const entriesSize = sizeof(Entry) * count;
        auto const allocChainOffset = flare::base::align_up(entriesSize, alignof(AllocationChain));
        auto const allocSize = allocChainOffset + sizeof(AllocationChain);

        allocation::required_info request;
        request.size = allocSize;
        request.alignment = std::max(alignof(Entry), alignof(AllocationChain));
        request.usage = allocation::Usage::List;
        auto alloca = alloc->allocate(request);

        auto entries = reinterpret_cast<Entry *>(alloca.ptr);
        for (size_t i = 0; i < count; i++) {
            auto entry = &entries[i];
            entry->prev = nullptr;
            entry->next = free;
            if (free) {
                free->prev = entry;
            }
            free = entry;
        }

        auto allocChain = reinterpret_cast<AllocationChain *>(
                reinterpret_cast<uint8_t *>(alloca.ptr) + allocChainOffset);

        allocChain->_allocation = alloca;
        allocChain->next = allocations;
        allocations = allocChain;

        capacity += count;
    }

    template<typename T>
    void list<T>::unlink(Entry *entry, Entry *&list) {
        if (list == entry) {
            list = list->next;
        }
        if (entry->prev) {
            entry->prev->next = entry->next;
        }
        if (entry->next) {
            entry->next->prev = entry->prev;
        }
        entry->prev = nullptr;
        entry->next = nullptr;
    }

    template<typename T>
    void list<T>::link(Entry *entry, Entry *&list) {
        FLARE_CHECK(entry->next == nullptr);
        FLARE_CHECK(entry->prev == nullptr);
        if (list) {
            entry->next = list;
            list->prev = entry;
        }
        list = entry;
    }

}  // namespace flare

#endif  // FLARE_CONTAINER_LIST_H_
