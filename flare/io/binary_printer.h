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

// Date: Thu Nov 22 13:57:56 CST 2012

#ifndef FLARE_IO_BINARY_PRINTER_H_
#define FLARE_IO_BINARY_PRINTER_H_

#include <string_view>

namespace flare::io {
    class cord_buf;

    // Print binary content within max length.
    // The printing format is optimized for humans and may change in future.

    class to_printable {
    public:
        static const size_t DEFAULT_MAX_LENGTH = 64;

        to_printable(const cord_buf &b, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(&b), _max_length(max_length) {}

        to_printable(const std::string_view &str, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(NULL), _str(str), _max_length(max_length) {}

        to_printable(const void *data, size_t n, size_t max_length = DEFAULT_MAX_LENGTH)
                : _iobuf(NULL), _str((const char *) data, n), _max_length(max_length) {}

        void print(std::ostream &os) const;

    private:
        const cord_buf *_iobuf;
        std::string_view _str;
        size_t _max_length;
    };

    // Keep old name for compatibility.
    typedef to_printable PrintedAsBinary;

    inline std::ostream &operator<<(std::ostream &os, const to_printable &p) {
        p.print(os);
        return os;
    }

    // Convert binary data to a printable string.
    std::string to_printable_string(const cord_buf &data,
                                  size_t max_length = to_printable::DEFAULT_MAX_LENGTH);

    std::string to_printable_string(const std::string_view &data,
                                  size_t max_length = to_printable::DEFAULT_MAX_LENGTH);

    std::string to_printable_string(const void *data, size_t n,
                                  size_t max_length = to_printable::DEFAULT_MAX_LENGTH);

} // namespace flare::io

#endif  // FLARE_IO_BINARY_PRINTER_H_
