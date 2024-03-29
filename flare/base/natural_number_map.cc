/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <flare/container/dynamic_bitset.h>
#include <flare/log/logging.h>
#include "flare/base/natural_number_map.h"

namespace flare {
    static constexpr auto invalid_position = flare::dynamic_bitset<>::npos;

    template<typename Key, typename Value>
    natural_number_map<Key, Value>::natural_number_map()
            : _size(0), _values_bitset(std::make_unique<flare::dynamic_bitset<>>()) {
    }

    template<typename Key, typename Value>
    void natural_number_map<Key, Value>::reserve(size_t count) {
        _values_vector.reserve(count);
        _values_bitset->reserve(count);
    }

    template<typename Key, typename Value>
    size_t natural_number_map<Key, Value>::size() const {
        return _size;
    }

    template<typename Key, typename Value>
    void natural_number_map<Key, Value>::set(Key key, Value value) {
        if (key >= _values_bitset->size()) {
            _values_bitset->resize(static_cast<size_t>(key) + 1);
            _values_vector.resize(_values_bitset->size());
        }

        _values_vector[key] = value;
        const bool was_present = _values_bitset->test_set(key, true);

        if (!was_present) {
            ++_size;
        }
    }

    template<typename Key, typename Value>
    void natural_number_map<Key, Value>::erase(Key key) {
        if (key < _values_bitset->size()) {
            const bool was_present = _values_bitset->test_set(key, false);

            if (was_present) {
                --_size;
            }
        }
    }

    template<typename Key, typename Value>
    bool natural_number_map<Key, Value>::contains(Key key) const {
        return key < _values_bitset->size() && _values_bitset->test(key);
    }

    template<typename Key, typename Value>
    bool natural_number_map<Key, Value>::try_get(Key key, Value &value) const {
        if (!contains(key)) {
            return false;
        }

        value = _values_vector[key];
        return true;
    }

    template<typename Key, typename Value>
    typename natural_number_map<Key, Value>::position
    natural_number_map<Key, Value>::find_first() const {
        return position{_size > 0 ? _values_bitset->find_first() : invalid_position,
                        0};
    }

    template<typename Key, typename Value>
    typename natural_number_map<Key, Value>::position
    natural_number_map<Key, Value>::find_next(
            const position &after_position) const {
        return position{after_position._keys_already_enumerated < _size
                        ? _values_bitset->find_next(after_position._key)
                        : invalid_position,
                        after_position._keys_already_enumerated + 1};
    }

    template<typename Key, typename Value>
    bool natural_number_map<Key, Value>::position::is_valid() const {
        return _key != invalid_position;
    }

    template<typename Key, typename Value>
    Value natural_number_map<Key, Value>::get(const position &pos) const {
        FLARE_CHECK(pos.is_valid());
        return _values_vector[pos._key];
    }

    template<typename Key, typename Value>
    void natural_number_map<Key, Value>::clear() {
        _size = 0;
        _values_vector.clear();
        _values_bitset->clear();
    }

    // Instantiate used templates.
    template
    class natural_number_map<unsigned, int32_t>;

    template
    class natural_number_map<unsigned, uint32_t>;

    template
    class natural_number_map<unsigned, int64_t>;

    template
    class natural_number_map<unsigned, uint64_t>;
}  // namespace flare
