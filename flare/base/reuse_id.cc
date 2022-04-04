//
// Created by liyinbin on 2022/4/4.
//

#include "flare/base/reuse_id.h"

namespace flare {

    std::size_t reuse_id::next() {
        std::scoped_lock lk(_mutex);
        if (!_recycled.empty()) {
            auto rc = _recycled.back();
            _recycled.pop_back();
            return rc;
        }
        return _current++;
    }

    void reuse_id::free(std::size_t index) {
        std::scoped_lock lk(_mutex);
        if(index + 1 == _current) {
            _current--;
            return;
        }
        _recycled.push_back(index);
    }

    // no need to lock, for it us call by free, and locked by the
    // outside.
    void reuse_id::do_shrink() {
        std::sort(_recycled.begin(), _recycled.end());
        while(_recycled.back() + 1 == _current) {
            _recycled.pop_back();
            _current--;
        }
    }

}  // namespace flare