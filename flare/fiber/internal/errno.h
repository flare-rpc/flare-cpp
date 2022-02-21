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

// fiber - A M:N threading library to make applications more concurrent.

// Date: Wed Jul 30 11:47:19 CST 2014

#ifndef FLARE_FIBER_INTERNAL_ERRNO_H_
#define FLARE_FIBER_INTERNAL_ERRNO_H_

#include <errno.h>                    // errno
#include "flare/base/errno.h"                // flare_error(),

__BEGIN_DECLS

extern int *fiber_errno_location();

#ifdef errno
#undef errno
#define errno *fiber_errno_location()
#endif

// List errno used throughout fiber
extern const int ESTOP;

__END_DECLS

#endif  // FLARE_FIBER_INTERNAL_ERRNO_H_
