//
// Created by liyinbin on 2022/1/19.
//

#ifndef FLARE_BUTIL_STATIC_ATOMIC_H_
#define FLARE_BUTIL_STATIC_ATOMIC_H_

#include <atomic>
#include "butil/macros.h"

#define FLARE_STATIC_ATOMIC_INIT(val) { (val) }

namespace flare {

    template<typename T>
    struct static_atomic {
        T val;

        // NOTE: the memory_order parameters must be present.
        T load(memory_order o) { return ref().load(o); }

        void store(T v, memory_order o) { return ref().store(v, o); }

        T exchange(T v, memory_order o) { return ref().exchange(v, o); }

        bool compare_exchange_weak(T &e, T d, memory_order o) { return ref().compare_exchange_weak(e, d, o); }

        bool compare_exchange_weak(T &e, T d, memory_order so, memory_order fo) {
            return ref().compare_exchange_weak(e, d, so, fo);
        }

        bool compare_exchange_strong(T &e, T d, memory_order o) { return ref().compare_exchange_strong(e, d, o); }

        bool compare_exchange_strong(T &e, T d, memory_order so, memory_order fo) {
            return ref().compare_exchange_strong(e, d, so, fo);
        }

        T fetch_add(T v, memory_order o) { return ref().fetch_add(v, o); }

        T fetch_sub(T v, memory_order o) { return ref().fetch_sub(v, o); }

        T fetch_and(T v, memory_order o) { return ref().fetch_and(v, o); }

        T fetch_or(T v, memory_order o) { return ref().fetch_or(v, o); }

        T fetch_xor(T v, memory_order o) { return ref().fetch_xor(v, o); }

        static_atomic &operator=(T v) {
            store(v, memory_order_seq_cst);
            return *this;
        }

    private:
        DISALLOW_ASSIGN(static_atomic);
        BAIDU_CASSERT(sizeof(T) == sizeof(std::atomic <T>), size_must_match);

        std::atomic <T> &ref() {
            // Suppress strict-alias warnings.
            std::atomic <T> *p = reinterpret_cast<std::atomic <T> *>(&val);
            return *p;
        }
    };
}  // namespace flare
#endif  // FLARE_BUTIL_STATIC_ATOMIC_H_
