
#ifndef FLARE_BASE_FILES_SCOPED_FILE_H_
#define FLARE_BASE_FILES_SCOPED_FILE_H_

#include <stdio.h>

#include "flare/log/logging.h"
#include "flare/base/scoped_generic.h"
#include "flare/base/profile.h"

namespace flare::base {

namespace internal {

#if defined(FLARE_PLATFORM_POSIX)
struct FLARE_EXPORT ScopedFDCloseTraits {
  static int InvalidValue() {
    return -1;
  }
  static void Free(int fd);
};
#endif

}  // namespace internal

// -----------------------------------------------------------------------------

#if defined(FLARE_PLATFORM_POSIX)
typedef scoped_generic<int, internal::ScopedFDCloseTraits> ScopedFD;
#endif

//// Automatically closes |FILE*|s.
class scoped_file {
    MOVE_ONLY_TYPE_FOR_CPP_03(scoped_file, RValue);
public:
    scoped_file() : _fp(NULL) {}

    // Open file at |path| with |mode|.
    // If fopen failed, operator FILE* returns NULL and errno is set.
    scoped_file(const char *path, const char *mode) {
        _fp = fopen(path, mode);
    }

    // |fp| must be the return value of fopen as we use fclose in the
    // destructor, otherwise the behavior is undefined
    explicit scoped_file(FILE *fp) :_fp(fp) {}

    scoped_file(RValue rvalue) {
        _fp = rvalue.object->_fp;
        rvalue.object->_fp = NULL;
    }

    ~scoped_file() {
        if (_fp != NULL) {
            fclose(_fp);
            _fp = NULL;
        }
    }

    // Close current opened file and open another file at |path| with |mode|
    void reset(const char* path, const char* mode) {
        reset(fopen(path, mode));
    }

    void reset() { reset(NULL); }

    void reset(FILE *fp) {
        if (_fp != NULL) {
            fclose(_fp);
            _fp = NULL;
        }
        _fp = fp;
    }

    // Set internal FILE* to NULL and return previous value.
    FILE* release() {
        FILE* const prev_fp = _fp;
        _fp = NULL;
        return prev_fp;
    }
    
    operator FILE*() const { return _fp; }

    FILE* get() { return _fp; }
    
private:
    FILE* _fp;
};

}  // namespace flare::base

#endif  // FLARE_BASE_FILES_SCOPED_FILE_H_
