//
// Created by liyinbin on 2022/4/17.
//

#ifndef FLARE_METRICS_EXPOSE_METRICS_H_
#define FLARE_METRICS_EXPOSE_METRICS_H_

#include <string>
#include <ostream>
#include <unordered_map>

namespace flare {

    struct metrics_family {
        std::unordered_map<std::string, std::string> tags;
        std::string prefix;
    };

    class expose_metrics {
    public:
        typedef std::unordered_map<std::string, std::string> tag_type;
    public:
        expose_metrics() = default;

        virtual ~expose_metrics() = default;

        const std::string &prefix() const {
            return _family.prefix;
        }

        const tag_type &tags() const {
            return _family.tags;
        }

        bool expose();

        bool hide();

        virtual void dump(std::ostream &out) const = 0;

    private:
        metrics_family _family;
    };
}  // namespace flare

#endif  // FLARE_METRICS_EXPOSE_METRICS_H_
