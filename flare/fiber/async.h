//
// Created by liyinbin on 2022/2/22.
//

#ifndef FLARE_FIBER_ASYNC_H_
#define FLARE_FIBER_ASYNC_H_

#include <type_traits>
#include <utility>
#include "flare/future/future.h"
#include "flare/fiber/fiber.h"

namespace flare {

    // Runs `f` with `args...` asynchronously.
    //
    // It's unspecified in which fiber (except the caller's own one) `f` is called.
    //
    // Note that this method is only available in fiber runtime. If you want to
    // start a fiber from pthread, use `start_fiber_from_pthread` (@sa: `fiber.h`)
    // instead.
    template <class F, class... Args,
            class R = future_internal::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
    R fiber_async(launch_policy policy, F&& f, Args&&... args) {
        flare::future_internal::as_promise_t<R> p;
        auto rc = p.get_future();

        // @sa: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0780r2.html
        auto proc = [p = std::move(p), f = std::forward<F>(f),
                args = std::make_tuple(std::forward<Args>(args)...)](void*) mutable ->void* {
            if constexpr (std::is_same_v<future<>, R>) {
                std::apply(f, std::move(args));
                p.set_value();
            } else {
                p.set_value(std::apply(f, std::move(args)));
            }
            return nullptr;
        };
        fiber fb = fiber(policy, std::move(proc), nullptr);
        fb.detach();
        return rc;
    }

    template <class F, class... Args,
            class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    auto fiber_async(F&& f, Args&&... args) {
        return fiber_async(launch_policy::eLazy, std::forward<F>(f), std::forward<Args>(args)...);
    }

}  // namespace flare
#endif // FLARE_FIBER_ASYNC_H_
