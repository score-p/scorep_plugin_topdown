#include <cstdint>

#include <scorep/SCOREP_MetricTypes.h>
#include <scorep/plugin/plugin.hpp>

#include <metric.hpp>

class topdown_plugin :
    // todo: object ID policy
    public scorep::plugin::base<topdown_plugin,
                                scorep::plugin::policy::sync,
                                scorep::plugin::policy::per_thread,
                                scorep::plugin::policy::scorep_clock,
                                tmam_metric_t_policy> {
public:
    void add_metric(const tmam_metric_t& h) {
        // todo
    }

    template <class Proxy>
    bool get_optional_value(const tmam_metric_t& id, Proxy& p) {
        return false;
    }

    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string&) {
        return {};
    }
};
