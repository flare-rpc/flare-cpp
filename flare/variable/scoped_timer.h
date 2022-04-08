// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: Fri Jul 14 11:29:21 CST 2017

#ifndef  FLARE_VARIABLE_SCOPED_TIMER_H_
#define  FLARE_VARIABLE_SCOPED_TIMER_H_

#include "flare/times/time.h"

// Accumulate microseconds spent by scopes into variable, useful for debugging.
// Example:
//   flare::variable::Adder<int64_t> g_function1_spent;
//   ...
//   void function1() {
//     // time cost by function1() will be sent to g_spent_time when
//     // the function returns.
//     flare::variable::ScopedTimer tm(g_function1_spent);
//     ...
//   }
// To check how many microseconds the function spend in last second, you
// can wrap the variable within PerSecond and make it viewable from /vars
//   flare::variable::PerSecond<flare::variable::Adder<int64_t> > g_function1_spent_second(
//     "function1_spent_second", &g_function1_spent);
namespace flare::variable {
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
} // namespace flare::variable

#endif  // FLARE_VARIABLE_SCOPED_TIMER_H_
