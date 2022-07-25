#include <scorep/SCOREP_MetricTypes.h>
#include <scorep/plugin/plugin.hpp>

class topdown_plugin :
    // todo: object ID policy
    public scorep::plugin::base<topdown_plugin, scorep::plugin::policy::sync, scorep::plugin::policy::per_thread, scorep::plugin::policy::scorep_clock> {
public:

    template <class Proxy>
    bool get_optional_value(std::int32_t id, Proxy& p) {
        return false;
    }

    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string&) {
        return {};
    }

    int32_t add_metric(const std::string&) {
        return 0;
    }
};
