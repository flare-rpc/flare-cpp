//
// Created by liyinbin on 2022/2/15.
//

#ifndef FLARE_THREAD_LATCH_H_
#define FLARE_THREAD_LATCH_H_

#include <condition_variable>
#include <mutex>
#include "flare/times/time.h"
#include "flare/log/logging.h"

namespace flare {

    class latch {
    public:
        explicit latch(std::ptrdiff_t count);

        // Decrement internal counter. If it reaches zero, wake up all waiters.
        void count_down(std::ptrdiff_t update = 1);

        // Test if the latch's internal counter has become zero.
        bool try_wait() const noexcept;

        // Wait until `latch`'s internal counter reached zero.
        void wait() const;

        bool wait_for(const flare::duration &d) {
            std::chrono::microseconds timeout = d.to_chrono_microseconds();
            std::unique_lock lk(m_);
            FLARE_CHECK_GE(count_, 0);
            return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
        }

        bool wait_until(const flare::time_point &deadline) {
            auto d = deadline.to_chrono_time();
            std::unique_lock lk(m_);
            FLARE_CHECK_GE(count_, 0);
            return cv_.wait_until(lk, d, [this] { return count_ == 0; });
        }

        // Shorthand for `count_down(); wait();`
        void arrive_and_wait(std::ptrdiff_t update = 1);

    private:
        mutable std::mutex m_;
        mutable std::condition_variable cv_;
        std::ptrdiff_t count_;
    };

}  // namespace flare
#endif  // FLARE_THREAD_LATCH_H_
