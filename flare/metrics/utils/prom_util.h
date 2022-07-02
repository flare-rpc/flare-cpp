//
// Created by liyinbin on 2022/7/1.
//

#ifndef FLARE_METRICS_UTILS_PROM_UTIL_H_
#define FLARE_METRICS_UTILS_PROM_UTIL_H_

#include <ostream>
#include <string>
#include "flare/times/time.h"

namespace flare {

    namespace metrics_detail {

        inline void write_value(std::ostream &out, double value) {
            if (std::isnan(value)) {
                out << "Nan";
            } else if (std::isinf(value)) {
                out << (value < 0 ? "-Inf" : "+Inf");
            } else {
                auto saved_flags = out.setf(std::ios::fixed, std::ios::floatfield);
                out << value;
                out.setf(saved_flags, std::ios::floatfield);
            }
        }

        inline void write_value(std::ostream &out, const std::string &value) {
            for (auto c : value) {
                if (c == '\\' || c == '"' || c == '\n') {
                    out << "\\";
                }
                out << c;
            }
        }

        inline void write_tail(std::ostream &out, const flare::time_point *t) {
            if (t) {
                out << " " << t->to_unix_millis();
            }
            out << "\n";
        }

        template<typename T = std::string>
        void write_head(std::ostream &out, const std::string &name,
                       const std::unordered_map<std::string, std::string> & tags,
                       const std::string &suffix = "",
                       const std::string &extraLabelName = "",
                       const T &extraLabelValue = T()) {
            out << name << suffix;
            if (!tags.empty() || !extraLabelName.empty()) {
                out << "{";
                const char *prefix = "";

                for (auto it = tags.begin(); it != tags.end(); it++) {
                    out << prefix << it->first << "=\"";
                    write_value(out, it->second);
                    out << "\"";
                    prefix = ",";
                }
                if (!extraLabelName.empty()) {
                    out << prefix << extraLabelName << "=\"";
                    write_value(out, extraLabelValue);
                    out << "\"";
                }
                out << "}";
            }
            out << " ";
        }

    }  // namespace metrics_detail
} // namespace flare
#endif  // FLARE_METRICS_UTILS_PROM_UTIL_H_
