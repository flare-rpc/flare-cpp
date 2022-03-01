//
// Created by liyinbin on 2022/2/21.
//

#ifndef FLARE_FIBER_FUTURE_H_
#define FLARE_FIBER_FUTURE_H_

#include <memory>
#include <utility>
#include <optional>
#include "flare/memory/lazy_init.h"
#include "flare/fiber/fiber_latch.h"
#include "flare/future/future.h"

namespace flare {

    template <class... Ts>
    auto fiber_future_get(future<Ts...>&& f) {
        fiber_latch l;
        flare::lazy_init<future_internal::boxed<Ts...>> receiver;

        // Once the `future` is satisfied, our continuation will move the
        // result into `receiver` and notify `cv` to wake us up.
        std::move(f).then([&](future_internal::boxed<Ts...> boxed) noexcept {
            // Do we need some synchronization primitives here to protect `receiver`?
            // `Event` itself guarantees `Set()` happens-before `Wait()` below, so
            // writing to `receiver` is guaranteed to happens-before reading it.
            //
            // But OTOH, what guarantees us that initialization of `receiver`
            // happens-before our assignment to it? `future::Then`?
            receiver.init(std::move(boxed));
            l.signal();
        });

        l.wait();
        return std::move(*receiver).get();
    }

    // Same as `fiber_blocking_get` but this one accepts a timeout.
    template <class... Ts>
    auto fiber_future_try_get_until(future<Ts...>&& future, const timespec *timeout) {
        struct wait_state {
            fiber_latch event;

            // Protects `receiver`.
            //
            // Unlike `fiber_blocking_get`, here it's possible that after `event.Wait()` times
            // out, concurrently the future is satisfied. In this case the continuation
            // of the future will be racing with us on `receiver`.
            std::mutex lock;
            std::optional<future_internal::boxed<Ts...>> receiver;

        };
        auto state = std::make_shared<wait_state>();

        // `state` must be copied here, in case of timeout, we'll leave the scope
        // before continuation is fired.
        std::move(future).then([state](future_internal::boxed<Ts...> boxed) noexcept {
            std::scoped_lock _(state->lock);
            state->receiver.emplace(std::move(boxed));
            state->event.signal();
        });

        state->event.timed_wait(timeout);
        std::scoped_lock _(state->lock);
        if constexpr (sizeof...(Ts) == 0) {
            return !!state->receiver;
        } else {
            return state->receiver ? std::optional(std::move(*state->receiver).get()) : std::nullopt;
        }
    }


    template <class... Ts>
    auto fiber_future_get(future<Ts...>* f) {
        return fiber_future_get(std::move(*f));
    }

    template <class... Ts>
    auto fiber_future_try_get_until(future<Ts...>* f,const timespec *timeout) {
        return fiber_future_try_get_until(std::move(*f), timeout);
    }

}  // namespace flare

#endif // FLARE_FIBER_FUTURE_H_
