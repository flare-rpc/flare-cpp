
#ifndef FLARE_BASE_STATUS_H_
#define FLARE_BASE_STATUS_H_

#include <stdarg.h>                       // va_list
#include <stdlib.h>                       // free
#include <string>                         // std::string
#include <ostream>                        // std::ostream
#include <string_view>

namespace flare::base {

    // A flare_status encapsulates the result of an operation. It may indicate success,
    // or it may indicate an error with an associated error message. It's suitable
    // for passing status of functions with richer information than just error_code
    // in exception-forbidden code. This utility is inspired by leveldb::flare_status.
    //
    // Multiple threads can invoke const methods on a flare_status without
    // external synchronization, but if any of the threads may call a
    // non-const method, all threads accessing the same flare_status must use
    // external synchronization.
    //
    // Since failed status needs to allocate memory, you should be careful when
    // failed status is frequent.

    class flare_status {
    public:
        struct State {
            int code;
            unsigned size;  // length of message string
            unsigned state_size;
            char message[0];
        };

        // Create a success status.
        flare_status() : _state(NULL) {}

        // Return a success status.
        static flare_status OK() { return flare_status(); }

        ~flare_status() { reset(); }

        // Create a failed status.
        // error_text is formatted from `fmt' and following arguments.
        flare_status(int code, const char *fmt, ...)
        __attribute__ ((__format__ (__printf__, 3, 4)))
                : _state(NULL) {
            va_list ap;
            va_start(ap, fmt);
            set_errorv(code, fmt, ap);
            va_end(ap);
        }

        flare_status(int code, const std::string_view &error_msg) : _state(NULL) {
            set_error(code, error_msg);
        }

        // Copy the specified status. Internal fields are deeply copied.
        flare_status(const flare_status &s);

        void operator=(const flare_status &s);

        // Reset this status to be OK.
        void reset();

        // Reset this status to be failed.
        // Returns 0 on success, -1 otherwise and internal fields are not changed.
        int set_error(int code, const char *error_format, ...)
        __attribute__ ((__format__ (__printf__, 3, 4)));

        int set_error(int code, const std::string_view &error_msg);

        int set_errorv(int code, const char *error_format, va_list args);

        // Returns true iff the status indicates success.
        bool ok() const { return (_state == NULL); }

        // Get the error code
        int error_code() const {
            return (_state == NULL) ? 0 : _state->code;
        }

        // Return a string representation of the status.
        // Returns "OK" for success.
        // NOTICE:
        //   * You can print a flare_status to std::ostream directly
        //   * if message contains '\0', error_cstr() will not be shown fully.
        const char *error_cstr() const {
            return (_state == NULL ? "OK" : _state->message);
        }

        std::string_view error_data() const {
            return (_state == NULL ? std::string_view("OK", 2)
                                   : std::string_view(_state->message, _state->size));
        }

        std::string error_str() const;

        void swap(flare_status &other) { std::swap(_state, other._state); }

    private:
        // OK status has a NULL _state.  Otherwise, _state is a State object
        // converted from malloc().
        State *_state;

        static State *copy_state(const State *s);
    };

    inline flare_status::flare_status(const flare_status &s) {
        _state = (s._state == NULL) ? NULL : copy_state(s._state);
    }

    inline int flare_status::set_error(int code, const char *msg, ...) {
        va_list ap;
        va_start(ap, msg);
        const int rc = set_errorv(code, msg, ap);
        va_end(ap);
        return rc;
    }

    inline void flare_status::reset() {
        free(_state);
        _state = NULL;
    }

    inline void flare_status::operator=(const flare_status &s) {
        // The following condition catches both aliasing (when this == &s),
        // and the common case where both s and *this are ok.
        if (_state == s._state) {
            return;
        }
        if (s._state == NULL) {
            free(_state);
            _state = NULL;
        } else {
            set_error(s._state->code,
                      std::string_view(s._state->message, s._state->size));
        }
    }

    inline std::ostream &operator<<(std::ostream &os, const flare_status &st) {
        // NOTE: don't use st.error_text() which is inaccurate if message has '\0'
        return os << st.error_data();
    }

}  // namespace flare::base

#endif  // FLARE_BASE_STATUS_H_
