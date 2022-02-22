//
// Created by liyinbin on 2021/4/4.
//

#ifndef FLARE_MEMORY_LAZY_INIT_H_
#define FLARE_MEMORY_LAZY_INIT_H_


#include <optional>
#include <utility>

#include "flare/log/logging.h"

namespace flare::memory {

    template<class T>
    class lazy_init {
    public:
        template<class... Args>
        void init(Args &&... args) {
            value_.emplace(std::forward<Args>(args)...);
        }

        void destroy() { value_ = std::nullopt; }

        T *operator->() {
            DCHECK(value_);
            return &*value_;
        }

        const T *operator->() const {
            DCHECK(value_);
            return &*value_;
        }

        T &operator*() {
            DCHECK(value_);
            return *value_;
        }

        const T &operator*() const {
            DCHECK(value_);
            return *value_;
        }

        explicit operator bool() const { return !!value_; }

    private:
        std::optional<T> value_;
    };

}  // namespace flare::memory

#endif  // FLARE_MEMORY_LAZY_INIT_H_
