//
// Created by liyinbin on 2022/4/17.
//

#ifndef FLARE_METRICS_SCOPE_GROUP_H_
#define FLARE_METRICS_SCOPE_GROUP_H_

#include <string>
#include<unordered_map>

namespace flare {

    class expose_metrics;

    class scope_group {
    public:

        scope_group *instance() {
            static scope_group inc;
            return &ins;
        }

        bool add(const std::string &prefix, std::unordered_map<std::string, std::string> &tag, expose_metrics* ptr);

        void list_metrics(std::vector<std::string> & res);

    private:
    };
}  // namespace flare

#endif  // FLARE_METRICS_SCOPE_GROUP_H_
