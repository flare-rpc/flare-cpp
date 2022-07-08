
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef  FLARE_BASE_POPEN_H_
#define  FLARE_BASE_POPEN_H_

#include <ostream>

namespace flare::base {

    // Read the stdout of child process executing `cmd'.
    // Returns the exit status(0-255) of cmd and all the output is stored in
    // |os|. -1 otherwise and errno is set appropriately.
    int read_command_output(std::ostream &os, const char *cmd);

}  // flare::base

#endif  // FLARE_BASE_POPEN_H_

