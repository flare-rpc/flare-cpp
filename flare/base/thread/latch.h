//
// Created by liyinbin on 2022/2/15.
//

#ifndef FLARE_BASE_THREAD_LATCHER_H_
#define FLARE_BASE_THREAD_LATCHER_H_

#include <condition_variable>
#include <mutex>
#include <chrono>
#include "flare/log/logging.h"

namespace flare::base {

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
            CHECK_GE(count_, 0);
            return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
        }

        bool wait_until(const flare::time_point &timeout) {
            //std::chrono::steady_clock::time_point deadline(std::chrono::nanoseconds(timeout_us * 1000L));
            auto deadline = timeout.to_chrono_time();
            std::unique_lock lk(m_);
            CHECK_GE(count_, 0);
            return cv_.wait_until(lk, deadline, [this] { return count_ == 0; });
        }

        // Shorthand for `count_down(); wait();`
        void arrive_and_wait(std::ptrdiff_t update = 1);

    private:
        mutable std::mutex m_;
        mutable std::condition_variable cv_;
        std::ptrdiff_t count_;
    };

}  // namespace flare::base
#endif  // FLARE_BASE_THREAD_LATCHER_H_
