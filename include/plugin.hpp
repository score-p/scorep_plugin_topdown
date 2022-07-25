#include <cstdint>
#include <memory>
#include <stdexcept>

#include <scorep/SCOREP_MetricTypes.h>
#include <scorep/plugin/plugin.hpp>

#include <metric.hpp>
#include <perf_util.hpp>

class topdown_plugin :
    // todo: object ID policy
    public scorep::plugin::base<topdown_plugin,
                                scorep::plugin::policy::sync,
                                scorep::plugin::policy::per_thread,
                                scorep::plugin::policy::scorep_clock,
                                tmam_metric_t_policy> {
private:
    /// handle to TMAM perf wrapper
    std::shared_ptr<perf_tmam_handle> tmam_handle;

public:
    topdown_plugin() : tmam_handle(nullptr) {};

    void add_metric(const tmam_metric_t&) {
        // record given metric for *current thread*
        // note that there is no specific procedure for an individual metric,
        // all metrics are recorded by the same object
        // -> init tmam measurement if not done already
        if (!tmam_handle) {
            tmam_handle = std::make_shared<perf_tmam_handle>();
        }
    }

    template <class Proxy>
    bool get_optional_value(const tmam_metric_t& id, Proxy& p) {
        return false;
    }

    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& pattern) {
        if ("*" != pattern) {
            throw std::runtime_error("pattern must be '*'");
        }

        std::vector<scorep::plugin::metric_property> result;
        for (const auto& metric : tmam_metric_t::all) {
            make_handle(metric.get_name(), metric);
        }

        return result;
    }
};
