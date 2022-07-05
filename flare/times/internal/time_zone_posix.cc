
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/times/internal/time_zone_posix.h"
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include "flare/base/profile.h"

namespace flare::times_internal {

    namespace {

        const char kDigits[] = "0123456789";

        const char *parse_int(const char *p, int min, int max, int *vp) {
            int value = 0;
            const char *op = p;
            const int kMaxInt = std::numeric_limits<int>::max();
            for (; const char *dp = strchr(kDigits, *p); ++p) {
                int d = static_cast<int>(dp - kDigits);
                if (d >= 10)
                    break;  // '\0'
                if (value > kMaxInt / 10)
                    return nullptr;
                value *= 10;
                if (value > kMaxInt - d)
                    return nullptr;
                value += d;
            }
            if (p == op || value < min || value > max)
                return nullptr;
            *vp = value;
            return p;
        }

// abbr = <.*?> | [^-+,\d]{3,}
        const char *parse_abbr(const char *p, std::string *abbr) {
            const char *op = p;
            if (*p == '<') {  // special zoneinfo <...> form
                while (*++p != '>') {
                    if (*p == '\0')
                        return nullptr;
                }
                abbr->assign(op + 1, static_cast<std::size_t>(p - op) - 1);
                return ++p;
            }
            while (*p != '\0') {
                if (strchr("-+,", *p))
                    break;
                if (strchr(kDigits, *p))
                    break;
                ++p;
            }
            if (p - op < 3)
                return nullptr;
            abbr->assign(op, static_cast<std::size_t>(p - op));
            return p;
        }

// offset = [+|-]hh[:mm[:ss]] (aggregated into single seconds value)
        const char *parse_offset(const char *p, int min_hour, int max_hour, int sign,
                                 std::int_fast32_t *offset) {
            if (p == nullptr)
                return nullptr;
            if (*p == '+' || *p == '-') {
                if (*p++ == '-')
                    sign = -sign;
            }
            int hours = 0;
            int minutes = 0;
            int seconds = 0;

            p = parse_int(p, min_hour, max_hour, &hours);
            if (p == nullptr)
                return nullptr;
            if (*p == ':') {
                p = parse_int(p + 1, 0, 59, &minutes);
                if (p == nullptr)
                    return nullptr;
                if (*p == ':') {
                    p = parse_int(p + 1, 0, 59, &seconds);
                    if (p == nullptr)
                        return nullptr;
                }
            }
            *offset = sign * ((((hours * 60) + minutes) * 60) + seconds);
            return p;
        }

// datetime = ( Jn | n | Mm.w.d ) [ / offset ]
        const char *parse_date_time(const char *p, posix_transition *res) {
            if (p != nullptr && *p == ',') {
                if (*++p == 'M') {
                    int month = 0;
                    if ((p = parse_int(p + 1, 1, 12, &month)) != nullptr && *p == '.') {
                        int week = 0;
                        if ((p = parse_int(p + 1, 1, 5, &week)) != nullptr && *p == '.') {
                            int weekday = 0;
                            if ((p = parse_int(p + 1, 0, 6, &weekday)) != nullptr) {
                                res->date.fmt = posix_transition::M;
                                res->date.m.month = static_cast<std::int_fast8_t>(month);
                                res->date.m.week = static_cast<std::int_fast8_t>(week);
                                res->date.m.weekday = static_cast<std::int_fast8_t>(weekday);
                            }
                        }
                    }
                } else if (*p == 'J') {
                    int day = 0;
                    if ((p = parse_int(p + 1, 1, 365, &day)) != nullptr) {
                        res->date.fmt = posix_transition::J;
                        res->date.j.day = static_cast<std::int_fast16_t>(day);
                    }
                } else {
                    int day = 0;
                    if ((p = parse_int(p, 0, 365, &day)) != nullptr) {
                        res->date.fmt = posix_transition::N;
                        res->date.n.day = static_cast<std::int_fast16_t>(day);
                    }
                }
            }
            if (p != nullptr) {
                res->time.offset = 2 * 60 * 60;  // default offset is 02:00:00
                if (*p == '/')
                    p = parse_offset(p + 1, -167, 167, 1, &res->time.offset);
            }
            return p;
        }

    }  // namespace

// spec = std offset [ dst [ offset ] , datetime , datetime ]
    bool parse_posix_spec(const std::string &spec, posix_time_zone *res) {
        const char *p = spec.c_str();
        if (*p == ':')
            return false;

        p = parse_abbr(p, &res->std_abbr);
        p = parse_offset(p, 0, 24, -1, &res->std_offset);
        if (p == nullptr)
            return false;
        if (*p == '\0')
            return true;

        p = parse_abbr(p, &res->dst_abbr);
        if (p == nullptr)
            return false;
        res->dst_offset = res->std_offset + (60 * 60);  // default
        if (*p != ',')
            p = parse_offset(p, 0, 24, -1, &res->dst_offset);

        p = parse_date_time(p, &res->dst_start);
        p = parse_date_time(p, &res->dst_end);

        return p != nullptr && *p == '\0';
    }


}  // namespace flare::times_internal
