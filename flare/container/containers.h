
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_CONTAINERS_H_
#define FLARE_CONTAINER_CONTAINERS_H_

#include "flare/memory/allocator.h"

#include <algorithm>  // std::max
#include <cstddef>    // size_t
#include <utility>    // std::move

#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace flare {
    namespace containers {

        template<typename T>
        using deque = std::deque<T, stl_allocator<T>>;

        template<typename K, typename V, typename C = std::less<K>>
        using map = std::map<K, V, C, stl_allocator<std::pair<const K, V>>>;

        template<typename K, typename C = std::less<K>>
        using set = std::set<K, C, stl_allocator<K>>;

        template<typename K,
                typename V,
                typename H = std::hash<K>,
                typename E = std::equal_to<K>>
        using unordered_map =
        std::unordered_map<K, V, H, E, stl_allocator<std::pair<const K, V>>>;

        template<typename K, typename H = std::hash<K>, typename E = std::equal_to<K>>
        using unordered_set = std::unordered_set<K, H, E, stl_allocator<K>>;

        // take() takes and returns the front value from the deque.
        template<typename T>
        inline T take(deque<T> &queue) {
            auto out = std::move(queue.front());
            queue.pop_front();
            return out;
        }

        // take() takes and returns the first value from the unordered_set.
        template<typename T, typename H, typename E>
        inline T take(unordered_set<T, H, E> &set) {
            auto it = set.begin();
            auto out = std::move(*it);
            set.erase(it);
            return out;
        }
    }  // namespace containers
}  // namespace flare

#endif  // FLARE_CONTAINER_CONTAINERS_H_
