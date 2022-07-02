
#ifndef  FLARE_VARIABLE_SCOPED_TIMER_H_
#define  FLARE_VARIABLE_SCOPED_TIMER_H_

#include "flare/times/time.h"

// Accumulate microseconds spent by scopes into variable, useful for debugging.
// Example:
//   flare::Adder<int64_t> g_function1_spent;
//   ...
//   void function1() {
//     // time cost by function1() will be sent to g_spent_time when
//     // the function returns.
//     flare::ScopedTimer tm(g_function1_spent);
//     ...
//   }
// To check how many microseconds the function spend in last second, you
// can wrap the variable within PerSecond and make it viewable from /vars
//   flare::PerSecond<flare::Adder<int64_t> > g_function1_spent_second(
//     "function1_spent_second", &g_function1_spent);
namespace flare {
    template<typename T>
    class ScopedTimer {
    public:
        explicit ScopedTimer(T &variable)
                : _start_time(flare::get_current_time_micros()), _var(&variable) {}

        ~ScopedTimer() {
            *_var << (flare::get_current_time_micros() - _start_time);
        }

        void reset() { _start_time = flare::get_current_time_micros(); }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(ScopedTimer);

        int64_t _start_time;
        T *_var;
    };
} // namespace flare

#endif  // FLARE_VARIABLE_SCOPED_TIMER_H_
