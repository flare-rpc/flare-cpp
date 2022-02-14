
#include "flare/base/at_exit.h"
#include <stddef.h>
#include <ostream>
#include "flare/log/logging.h"

namespace flare::base {

    // Keep a stack of registered AtExitManagers.  We always operate on the most
    // recent, and we should never have more than one outside of testing (for a
    // statically linked version of this library).  Testing may use the shadow
    // version of the constructor, and if we are building a dynamic library we may
    // end up with multiple AtExitManagers on the same process.  We don't protect
    // this for thread-safe access, since it will only be modified in testing.
    static at_exit_manager *g_top_manager = NULL;

    at_exit_manager::at_exit_manager() : next_manager_(g_top_manager) {
        // If multiple modules instantiate AtExitManagers they'll end up living in this
        // module... they have to coexist.
#if !defined(COMPONENT_BUILD)
        DCHECK(!g_top_manager);
#endif
        g_top_manager = this;
    }

    at_exit_manager::~at_exit_manager() {
        if (!g_top_manager) {
            NOTREACHED() << "Tried to ~at_exit_manager without an at_exit_manager";
            return;
        }
        DCHECK_EQ(this, g_top_manager);

        process_callbacks_now();
        g_top_manager = next_manager_;
    }

    // static
    void at_exit_manager::register_callback(AtExitCallbackType func, void *param) {
        DCHECK(func);
        if (!g_top_manager) {
            NOTREACHED() << "Tried to register_callback without an at_exit_manager";
            return;
        }

        std::unique_lock<std::mutex> lock(g_top_manager->lock_);
        g_top_manager->stack_.push({func, param});
    }

    // static
    void at_exit_manager::process_callbacks_now() {
        if (!g_top_manager) {
            NOTREACHED() << "Tried to process_callbacks_now without an at_exit_manager";
            return;
        }

        std::unique_lock<std::mutex> lock(g_top_manager->lock_);

        while (!g_top_manager->stack_.empty()) {
            Callback task = g_top_manager->stack_.top();
            task.func(task.param);
            g_top_manager->stack_.pop();
        }
    }

    at_exit_manager::at_exit_manager(bool shadow) : next_manager_(g_top_manager) {
        DCHECK(shadow || !g_top_manager);
        g_top_manager = this;
    }

}  // namespace flare::base
