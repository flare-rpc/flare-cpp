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


#ifndef BRPC_REDIS_COMMAND_H
#define BRPC_REDIS_COMMAND_H

#include <memory>           // std::unique_ptr
#include <vector>
#include "flare/io/iobuf.h"
#include "flare/base/status.h"
#include "flare/memory/arena.h"
#include "flare/rpc/parse_result.h"

namespace flare::rpc {

// Format a redis command and append it to `buf'.
// Returns flare::base::flare_status::OK() on success.
flare::base::flare_status RedisCommandFormat(flare::io::IOBuf* buf, const char* fmt, ...);
flare::base::flare_status RedisCommandFormatV(flare::io::IOBuf* buf, const char* fmt, va_list args);

// Just convert the command to the text format of redis without processing the
// specifiers(%) inside.
flare::base::flare_status RedisCommandNoFormat(flare::io::IOBuf* buf, const std::string_view& command);

// Concatenate components to form a redis command.
flare::base::flare_status RedisCommandByComponents(flare::io::IOBuf* buf,
                                      const std::string_view* components,
                                      size_t num_components);

// A parser used to parse redis raw command.
class RedisCommandParser {
public:
    RedisCommandParser();

    // Parse raw message from `buf'. Return PARSE_OK and set the parsed command
    // to `args' and length to `len' if successful. Memory of args are allocated 
    // in `arena'.
    ParseError Consume(flare::io::IOBuf& buf, std::vector<std::string_view>* args,
                       flare::memory::Arena* arena);

private:
    // Reset parser to the initial state.
    void Reset();

    bool _parsing_array;            // if the parser has met array indicator '*'
    int _length;                    // array length
    int _index;                     // current parsing array index
    std::vector<std::string_view> _args;  // parsed command string
};

} // namespace flare::rpc


#endif  // BRPC_REDIS_COMMAND_H
