//
// Created by liyinbin on 2021/4/3.
//

#ifndef FLARE_FUTURE_FUTURE_H_
#define FLARE_FUTURE_FUTURE_H_

#include "flare/future/internal/future_impl.h"
#include "flare/future/internal/future.h"
#include "flare/future/internal/promise.h"
#include "flare/future/internal/promise_impl.h"
#include "flare/future/internal/future_utils.h"

namespace flare {


    using future_internal::future;
    using future_internal::futurize_tuple;
    using future_internal::futurize_values;
    using future_internal::make_ready_future;
    using future_internal::promise;
    using future_internal::split;
    using future_internal::when_all;
    using future_internal::when_any;
}  // namespace flare
#endif  // FLARE_FUTURE_FUTURE_H_
