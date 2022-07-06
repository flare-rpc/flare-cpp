
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

// An async-signal-safe and thread-safe demangler for Itanium C++ ABI
// (aka G++ V3 ABI).
//
// The demangler is implemented to be used in async signal handlers to
// symbolize stack traces.  We cannot use libstdc++'s
// abi::__cxa_demangle() in such signal handlers since it's not async
// signal safe (it uses malloc() internally).
//
// Note that this demangler doesn't support full demangling.  More
// specifically, it doesn't print types of function parameters and
// types of template arguments.  It just skips them.  However, it's
// still very useful to extract basic information such as class,
// function, constructor, destructor, and operator names.
//
// See the implementation note in demangle.cc if you are interested.
//
// Example:
//
// | Mangled Name  | The Demangler | abi::__cxa_demangle()
// |---------------|---------------|-----------------------
// | _Z1fv         | f()           | f()
// | _Z1fi         | f()           | f(int)
// | _Z3foo3bar    | foo()         | foo(bar)
// | _Z1fIiEvi     | f<>()         | void f<int>(int)
// | _ZN1N1fE      | N::f          | N::f
// | _ZN3Foo3BarEv | Foo::Bar()    | Foo::Bar()
// | _Zrm1XS_"     | operator%()   | operator%(X, X)
// | _ZN3FooC1Ev   | Foo::Foo()    | Foo::Foo()
// | _Z1fSs        | f()           | f(std::basic_string<char,
// |               |               |   std::char_traits<char>,
// |               |               |   std::allocator<char> >)
//
// See the unit test for more examples.
//
// Note: we might want to write demanglers for ABIs other than Itanium
// C++ ABI in the future.
//

#ifndef FLARE_DEBUGGING_INTERNAL_DEMANGLE_H_
#define FLARE_DEBUGGING_INTERNAL_DEMANGLE_H_

#include "flare/base/profile.h"

namespace flare::debugging {

    namespace debugging_internal {

        // Demangle `mangled`.  On success, return true and write the
        // demangled symbol name to `out`.  Otherwise, return false.
        // `out` is modified even if demangling is unsuccessful.
        bool Demangle(const char *mangled, char *out, int out_size);

    }  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_DEBUGGING_INTERNAL_DEMANGLE_H_
