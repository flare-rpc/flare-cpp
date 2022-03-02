// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLARE_BASE_AT_EXIT_H_
#define FLARE_BASE_AT_EXIT_H_

#include <stack>
#include <mutex>
#include "flare/base/profile.h"

namespace flare::base {

    // This class provides a facility similar to the CRT atexit(), except that
    // we control when the callbacks are executed. Under Windows for a DLL they
    // happen at a really bad time and under the loader lock. This facility is
    // mostly used by flare::Singleton.
    //
    // The usage is simple. Early in the main() or WinMain() scope create an
    // at_exit_manager object on the stack:
    // int main(...) {
    //    flare::base::at_exit_manager exit_manager;
    //
    // }
    // When the exit_manager object goes out of scope, all the registered
    // callbacks and singleton destructors will be called.

    class FLARE_EXPORT at_exit_manager {
    public:
        typedef void (*AtExitCallbackType)(void *);

        at_exit_manager();

        // The dtor calls all the registered callbacks. Do not try to register more
        // callbacks after this point.
        ~at_exit_manager();

        // Registers the specified function to be called at exit. The prototype of
        // the callback function is void func(void*).
        static void register_callback(AtExitCallbackType func, void *param);

        // Calls the functions registered with register_callback in LIFO order. It
        // is possible to register new callbacks after calling this function.
        static void process_callbacks_now();

    protected:
        // This constructor will allow this instance of at_exit_manager to be created
        // even if one already exists.  This should only be used for testing!
        // AtExitManagers are kept on a global stack, and it will be removed during
        // destruction.  This allows you to shadow another at_exit_manager.
        explicit at_exit_manager(bool shadow);

    private:
        struct Callback {
            AtExitCallbackType func;
            void *param;
        };
        std::mutex lock_;
        std::stack<Callback> stack_;
        at_exit_manager *next_manager_;  // Stack of managers to allow shadowing.

        FLARE_DISALLOW_COPY_AND_ASSIGN(at_exit_manager);
    };

#if defined(UNIT_TEST)
    class ShadowingAtExitManager : public at_exit_manager {
     public:
      ShadowingAtExitManager() : at_exit_manager(true) {}
    };
#endif  // defined(UNIT_TEST)

}  // namespace flare::base

#endif  // FLARE_BASE_AT_EXIT_H_
