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


#include "flare/log/logging.h"
#include "flare/rpc/log.h"
#include "flare/rpc/redis_command.h"

namespace flare::rpc {

    const size_t CTX_WIDTH = 5;

    // Much faster than snprintf(..., "%lu", d);
    inline size_t AppendDecimal(char *outbuf, unsigned long d) {
        char buf[24];  // enough for decimal 64-bit integers
        size_t n = sizeof(buf);
        do {
            const unsigned long q = d / 10;
            buf[--n] = d - q * 10 + '0';
            d = q;
        } while (d);
        fast_memcpy(outbuf, buf + n, sizeof(buf) - n);
        return sizeof(buf) - n;
    }

    // This function is the hotspot of RedisCommandFormatV() when format is
    // short or does not have many %. In a 100K-time call to formating of
    // "GET key1", the time spent on RedisRequest.AddCommand() are ~700ns
    // vs. ~400ns while using snprintf() vs. AppendDecimal() respectively.
    inline void AppendHeader(std::string &buf, char fc, unsigned long value) {
        char header[32];
        header[0] = fc;
        size_t len = AppendDecimal(header + 1, value);
        header[len + 1] = '\r';
        header[len + 2] = '\n';
        buf.append(header, len + 3);
    }

    inline void AppendHeader(flare::cord_buf &buf, char fc, unsigned long value) {
        char header[32];
        header[0] = fc;
        size_t len = AppendDecimal(header + 1, value);
        header[len + 1] = '\r';
        header[len + 2] = '\n';
        buf.append(header, len + 3);
    }

    static void FlushComponent(std::string *out, std::string *compbuf, int *ncomp) {
        AppendHeader(*out, '$', compbuf->size());
        out->append(*compbuf);
        out->append("\r\n", 2);
        compbuf->clear();
        ++*ncomp;
    }

    // Support hiredis-style format, namely everything is same with printf except
    // that %b corresponds to binary-data + length. Notice that we can't use
    // %.*s (printf built-in) which ends scaning at \0 and is not binary-safe.
    // Some code is copied or modified from redisvFormatCommand() in
    // https://github.com/redis/hiredis/blob/master/hiredis.c to keep close
    // compatibility with hiredis.
    flare::result_status
    RedisCommandFormatV(flare::cord_buf *outbuf, const char *fmt, va_list ap) {
        if (outbuf == nullptr || fmt == nullptr) {
            return flare::result_status(EINVAL, "Param[outbuf] or [fmt] is nullptr");
        }
        const size_t fmt_len = strlen(fmt);
        std::string nocount_buf;
        nocount_buf.reserve(fmt_len * 3 / 2 + 16);
        std::string compbuf;  // A component
        compbuf.reserve(fmt_len + 16);
        const char *c = fmt;
        int ncomponent = 0;
        char quote_char = 0;
        const char *quote_pos = fmt;
        int nargs = 0;
        for (; *c; ++c) {
            if (*c != '%' || c[1] == '\0') {
                if (*c == ' ') {
                    if (quote_char) {
                        compbuf.push_back(*c);
                    } else if (!compbuf.empty()) {
                        FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                    }
                } else if (*c == '"' || *c == '\'') {  // Check quotation.
                    if (!quote_char) {  // begin quote
                        quote_char = *c;
                        quote_pos = c;
                        if (!compbuf.empty()) {
                            FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                        }
                    } else if (quote_char == *c) {
                        const char last_char = (compbuf.empty() ? 0 : compbuf.back());
                        if (last_char == '\\') {
                            // Even if the preceding chars are two consecutive backslashes
                            // (\\), still do the escaping, which is the behavior of
                            // official redis-cli.
                            compbuf.pop_back();
                            compbuf.push_back(*c);
                        } else { // end quote
                            quote_char = 0;
                            FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                        }
                    } else {
                        compbuf.push_back(*c);
                    }
                } else {
                    compbuf.push_back(*c);
                }
            } else {
                char *arg;
                size_t size;

                switch (c[1]) {
                    case 's':
                        arg = va_arg(ap, char*);
                        size = strlen(arg);
                        if (size > 0) {
                            compbuf.append(arg, size);
                        }
                        ++nargs;
                        break;
                    case 'b':
                        arg = va_arg(ap, char*);
                        size = va_arg(ap, size_t);
                        if (size > 0) {
                            compbuf.append(arg, size);
                        }
                        ++nargs;
                        break;
                    case '%':
                        compbuf.push_back('%');
                        break;
                    default: {
                        /* Try to detect printf format */
                        static const char intfmts[] = "diouxX";
                        static const char flags[] = "#0-+ ";
                        char _format[24];
                        char _printed[40];
                        const char *_p = c + 1;
                        size_t _l = 0;
                        va_list _cpy;

                        /* Flags */
                        while (*_p != '\0' && strchr(flags, *_p) != nullptr) _p++;

                        /* Field width */
                        while (*_p != '\0' && isdigit(*_p)) _p++;

                        /* Precision */
                        if (*_p == '.') {
                            _p++;
                            while (*_p != '\0' && isdigit(*_p)) _p++;
                        }

                        /* Copy va_list before consuming with va_arg */
                        va_copy(_cpy, ap);

                        /* Integer conversion (without modifiers) */
                        if (strchr(intfmts, *_p) != nullptr) {
                            va_arg(ap, int);
                            goto fmt_valid;
                        }

                        /* Double conversion (without modifiers) */
                        if (strchr("eEfFgGaA", *_p) != nullptr) {
                            va_arg(ap, double);
                            goto fmt_valid;
                        }

                        /* Size: char */
                        if (_p[0] == 'h' && _p[1] == 'h') {
                            _p += 2;
                            if (*_p != '\0' && strchr(intfmts, *_p) != nullptr) {
                                va_arg(ap, int); /* char gets promoted to int */
                                goto fmt_valid;
                            }
                            goto fmt_invalid;
                        }

                        /* Size: short */
                        if (_p[0] == 'h') {
                            _p += 1;
                            if (*_p != '\0' && strchr(intfmts, *_p) != nullptr) {
                                va_arg(ap, int); /* short gets promoted to int */
                                goto fmt_valid;
                            }
                            goto fmt_invalid;
                        }

                        /* Size: long long */
                        if (_p[0] == 'l' && _p[1] == 'l') {
                            _p += 2;
                            if (*_p != '\0' && strchr(intfmts, *_p) != nullptr) {
                                va_arg(ap, long long);
                                goto fmt_valid;
                            }
                            goto fmt_invalid;
                        }

                        /* Size: long */
                        if (_p[0] == 'l') {
                            _p += 1;
                            if (*_p != '\0' && strchr(intfmts, *_p) != nullptr) {
                                va_arg(ap, long);
                                goto fmt_valid;
                            }
                            goto fmt_invalid;
                        }

                        fmt_invalid:
                        va_end(_cpy);
                        return flare::result_status(EINVAL, "Invalid format");

                        fmt_valid:
                        ++nargs;
                        _l = _p + 1 - c;
                        if (_l < sizeof(_format) - 2) {
                            memcpy(_format, c, _l);
                            _format[_l] = '\0';
                            int plen = vsnprintf(_printed, sizeof(_printed), _format, _cpy);
                            if (plen > 0) {
                                compbuf.append(_printed, plen);
                            }
                            /* Update current position (note: outer blocks
                             * increment c twice so compensate here) */
                            c = _p - 1;
                        }
                        va_end(_cpy);
                        break;
                    }  // end default
                }  // end switch

                ++c;
            }
        }
        if (quote_char) {
            const char *ctx_begin =
                    quote_pos - std::min((size_t) (quote_pos - fmt), CTX_WIDTH);
            size_t ctx_size =
                    std::min((size_t) (fmt + fmt_len - ctx_begin), CTX_WIDTH * 2 + 1);
            return flare::result_status(EINVAL, "Unmatched quote: ...{}... (offset={})",
                                        std::string_view(ctx_begin, (int) ctx_size), quote_pos - fmt);
        }

        if (!compbuf.empty()) {
            FlushComponent(&nocount_buf, &compbuf, &ncomponent);
        }

        FLARE_LOG_IF(ERROR, nargs == 0) << "You must call RedisCommandNoFormat() "
                                           "to replace RedisCommandFormatV without any args (to avoid potential "
                                           "formatting of conversion specifiers)";

        AppendHeader(*outbuf, '*', ncomponent);
        outbuf->append(nocount_buf);
        return flare::result_status::success();
    }

    flare::result_status RedisCommandFormat(flare::cord_buf *buf, const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        const flare::result_status st = RedisCommandFormatV(buf, fmt, ap);
        va_end(ap);
        return st;
    }

    flare::result_status
    RedisCommandNoFormat(flare::cord_buf *outbuf, const std::string_view &cmd) {
        if (outbuf == nullptr || cmd.empty()) {
            return flare::result_status(EINVAL, "Param[outbuf] or [cmd] is nullptr");
        }
        const size_t cmd_len = cmd.size();
        std::string nocount_buf;
        nocount_buf.reserve(cmd_len * 3 / 2 + 16);
        std::string compbuf;  // A component
        compbuf.reserve(cmd_len + 16);
        int ncomponent = 0;
        char quote_char = 0;
        const char *quote_pos = cmd.data();
        for (const char *c = cmd.data(); c != cmd.data() + cmd.size(); ++c) {
            if (*c == ' ') {
                if (quote_char) {
                    compbuf.push_back(*c);
                } else if (!compbuf.empty()) {
                    FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                }
            } else if (*c == '"' || *c == '\'') {  // Check quotation.
                if (!quote_char) {  // begin quote
                    quote_char = *c;
                    quote_pos = c;
                    if (!compbuf.empty()) {
                        FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                    }
                } else if (quote_char == *c) {
                    const char last_char = (compbuf.empty() ? 0 : compbuf.back());
                    if (last_char == '\\') {
                        // Even if the preceding chars are two consecutive backslashes
                        // (\\), still do the escaping, which is the behavior of
                        // official redis-cli.
                        compbuf.pop_back();
                        compbuf.push_back(*c);
                    } else { // end quote
                        quote_char = 0;
                        FlushComponent(&nocount_buf, &compbuf, &ncomponent);
                    }
                } else {
                    compbuf.push_back(*c);
                }
            } else {
                compbuf.push_back(*c);
            }
        }
        if (quote_char) {
            const char *ctx_begin =
                    quote_pos - std::min((size_t) (quote_pos - cmd.data()), CTX_WIDTH);
            size_t ctx_size =
                    std::min((size_t) (cmd.data() + cmd.size() - ctx_begin), CTX_WIDTH * 2 + 1);
            return flare::result_status(EINVAL, "Unmatched quote: ...{}... (offset={})",
                                             std::string_view(ctx_begin, (int) ctx_size), quote_pos - cmd.data());
        }

        if (!compbuf.empty()) {
            FlushComponent(&nocount_buf, &compbuf, &ncomponent);
        }

        AppendHeader(*outbuf, '*', ncomponent);
        outbuf->append(nocount_buf);
        return flare::result_status::success();
    }

    flare::result_status RedisCommandByComponents(flare::cord_buf *output,
                                                       const std::string_view *components,
                                                       size_t ncomponents) {
        if (output == nullptr) {
            return flare::result_status(EINVAL, "Param[output] is nullptr");
        }
        AppendHeader(*output, '*', ncomponents);
        for (size_t i = 0; i < ncomponents; ++i) {
            AppendHeader(*output, '$', components[i].size());
            output->append(components[i].data(), components[i].size());
            output->append("\r\n", 2);
        }
        return flare::result_status::success();
    }

    RedisCommandParser::RedisCommandParser()
            : _parsing_array(false), _length(0), _index(0) {}

    ParseError RedisCommandParser::Consume(flare::cord_buf &buf,
                                           std::vector<std::string_view> *args,
                                           flare::Arena *arena) {
        const char *pfc = (const char *) buf.fetch1();
        if (pfc == nullptr) {
            return PARSE_ERROR_NOT_ENOUGH_DATA;
        }
        // '*' stands for array "*<size>\r\n<sub-reply1><sub-reply2>..."
        if (!_parsing_array && *pfc != '*') {
            return PARSE_ERROR_TRY_OTHERS;
        }
        // '$' stands for bulk string "$<length>\r\n<string>\r\n"
        if (_parsing_array && *pfc != '$') {
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        char intbuf[32];  // enough for fc + 64-bit decimal + \r\n
        const size_t ncopied = buf.copy_to(intbuf, sizeof(intbuf) - 1);
        intbuf[ncopied] = '\0';
        const size_t crlf_pos = std::string_view(intbuf, ncopied).find("\r\n");
        if (crlf_pos == std::string_view::npos) {  // not enough data
            return PARSE_ERROR_NOT_ENOUGH_DATA;
        }
        char *endptr = nullptr;
        int64_t value = strtoll(intbuf + 1/*skip fc*/, &endptr, 10);
        if (endptr != intbuf + crlf_pos) {
            FLARE_LOG(ERROR) << '`' << intbuf + 1 << "' is not a valid 64-bit decimal";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (value < 0) {
            FLARE_LOG(ERROR) << "Invalid len=" << value << " in redis command";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (!_parsing_array) {
            buf.pop_front(crlf_pos + 2/*CRLF*/);
            _parsing_array = true;
            _length = value;
            _index = 0;
            _args.resize(value);
            return Consume(buf, args, arena);
        }
        FLARE_CHECK(_index < _length) << "a complete command has been parsed. "
                                         "impl of RedisCommandParser::Parse is buggy";
        const int64_t len = value;  // `value' is length of the string
        if (len < 0) {
            FLARE_LOG(ERROR) << "string in command is nil!";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (len > (int64_t) std::numeric_limits<uint32_t>::max()) {
            FLARE_LOG(ERROR) << "string in command is too long! max length=2^32-1,"
                                " actually=" << len;
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (buf.size() < crlf_pos + 2 + (size_t) len + 2/*CRLF*/) {
            return PARSE_ERROR_NOT_ENOUGH_DATA;
        }
        buf.pop_front(crlf_pos + 2/*CRLF*/);
        char *d = (char *) arena->allocate((len / 8 + 1) * 8);
        buf.cutn(d, len);
        d[len] = '\0';
        _args[_index] = std::string_view(d, len);
        if (_index == 0) {
            // convert it to lowercase when it is command name
            for (int i = 0; i < len; ++i) {
                d[i] = ::tolower(d[i]);
            }
        }
        char crlf[2];
        buf.cutn(crlf, sizeof(crlf));
        if (crlf[0] != '\r' || crlf[1] != '\n') {
            FLARE_LOG(ERROR) << "string in command is not ended with CRLF";
            return PARSE_ERROR_ABSOLUTELY_WRONG;
        }
        if (++_index < _length) {
            return Consume(buf, args, arena);
        }
        args->swap(_args);
        Reset();
        return PARSE_OK;
    }

    void RedisCommandParser::Reset() {
        _parsing_array = false;
        _length = 0;
        _index = 0;
        _args.clear();
    }

} // namespace flare::rpc
