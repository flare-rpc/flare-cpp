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


#ifndef FLARE_RPC_RELOADABLE_FLAGS_H_
#define FLARE_RPC_RELOADABLE_FLAGS_H_

// To flare developers: This is a header included by user, don't depend
// on internal structures, use opaque pointers instead.

#include <stdint.h>

// Register an always-true valiator to a gflag so that the gflag is treated as
// reloadable by flare. If a validator exists, abort the program.
// You should call this macro within global scope. for example:
//
//   DEFINE_int32(foo, 0, "blah blah");
//   FLARE_RPC_VALIDATE_GFLAG(foo, flare::rpc::PassValidate);
//
// This macro does not work for string-flags because they're thread-unsafe to
// modify directly. To emphasize this, you have to write the validator by
// yourself and use google::GetCommandLineOption() to acess the flag.
#define FLARE_RPC_VALIDATE_GFLAG(flag, validate_fn)                     \
    const int register_FLAGS_ ## flag ## _dummy                         \
                 __attribute__((__unused__)) =                          \
        ::flare::rpc::RegisterFlagValidatorOrDie(                       \
            &FLAGS_##flag, (validate_fn))


namespace flare::rpc {

    extern bool PassValidate(const char *, bool);

    extern bool PassValidate(const char *, int32_t);

    extern bool PassValidate(const char *, int64_t);

    extern bool PassValidate(const char *, uint64_t);

    extern bool PassValidate(const char *, double);

    extern bool PositiveInteger(const char *, int32_t);

    extern bool PositiveInteger(const char *, int64_t);

    extern bool NonNegativeInteger(const char *, int32_t);

    extern bool NonNegativeInteger(const char *, int64_t);

    extern bool RegisterFlagValidatorOrDie(const bool *flag,
                                           bool (*validate_fn)(const char *, bool));

    extern bool RegisterFlagValidatorOrDie(const int32_t *flag,
                                           bool (*validate_fn)(const char *, int32_t));

    extern bool RegisterFlagValidatorOrDie(const int64_t *flag,
                                           bool (*validate_fn)(const char *, int64_t));

    extern bool RegisterFlagValidatorOrDie(const uint64_t *flag,
                                           bool (*validate_fn)(const char *, uint64_t));

    extern bool RegisterFlagValidatorOrDie(const double *flag,
                                           bool (*validate_fn)(const char *, double));
} // namespace flare::rpc


#endif  // FLARE_RPC_RELOADABLE_FLAGS_H_
