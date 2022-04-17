//
// Created by liyinbin on 2022/4/17.
//

#ifndef FLARE_METRICS_EXPOSE_METRICS_H_
#define FLARE_METRICS_EXPOSE_METRICS_H_

#include <string>
#include <ostream>
#include <unordered_map>

namespace flare {

    class expose_metrics {
    public:
        typedef std::unordered_map<std::string, std::string> tag_type;
    public:
        expose_metrics() = default;

        virtual ~expose_metrics() = default;

        const std::string &prefix() const {
            return _prefix;
        }

        const tag_type &tags() const {
            return _tags;
        }

        bool expose();

        bool hide();

        virtual void dump(std::ostream &out) const;

    private:
        std::unordered_map<std::string, std::string> _tags;
        std::string _prefix;
    };
}  // namespace flare

#endif  // FLARE_METRICS_EXPOSE_METRICS_H_
