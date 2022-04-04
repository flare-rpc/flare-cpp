//
// Created by liyinbin on 2022/4/4.
//

#ifndef FLARE_BASE_REUSE_ID_H_
#define FLARE_BASE_REUSE_ID_H_

#include <cstddef>
#include <mutex>
#include <vector>
#include "flare/memory/resident.h"

namespace flare {

    class reuse_id {
    public:
        template<class Tag>
        static reuse_id *instance() {
            static resident_singleton<reuse_id> ia;
            return ia.get();
        }

        std::size_t next();

        void free(std::size_t id);

    private:
        reuse_id() = default;

        ~reuse_id() = default;

        void do_shrink();

    private:
        std::mutex _mutex;
        std::size_t _current{0};
        std::vector<std::size_t> _recycled;

    };
}  // namespace flare

#endif  // FLARE_BASE_REUSE_ID_H_
