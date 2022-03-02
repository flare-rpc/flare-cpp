// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com
//

#include "flare/strings/internal/ostringstream.h"

namespace flare {

namespace strings_internal {

OStringStream::Buf::int_type OStringStream::overflow(int c) {
    assert(s_);
    if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof()))
        s_->push_back(static_cast<char>(c));
    return 1;
}

std::streamsize OStringStream::xsputn(const char *s, std::streamsize n) {
    assert(s_);
    s_->append(s, n);
    return n;
}

}  // namespace strings_internal

}  // namespace flare
