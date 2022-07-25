#pragma once

#include <string>

#include <scorep/SCOREP_MetricTypes.h>
#include <scorep/plugin/plugin.hpp>

/**
 * category of tmam result.
 * One scalar which can be extracted from a tmam result
 */
enum class tmam_metric_category {
    slots,
    bottleneck,
    l1_retiring,
    l1_bad_speculation,
    l1_frontend_bound,
    l1_backend_bound,
    l2_light_ops,
    l2_heavy_ops,
    l2_branch_misprediction,
    l2_machine_clear,
    l2_fetch_latency,
    l2_fetch_bandwidth,
    l2_core_bound,
    l2_memory_bound,
};

/**
 * represents one metric recorded into a trace
 *
 * note: mainly required for scorep plugin wrapper (and to provide some convenience functions)
 */
class tmam_metric_t {
public:
    /// category represented
    tmam_metric_category category;

    tmam_metric_t (tmam_metric_category metric);

    /// list of all supported metrics
    static const std::vector<tmam_metric_t> all;
    
    /// get metric name for trace
    std::string get_name() const;
};

template <typename T, typename Policies>
using tmam_metric_t_policy = scorep::plugin::policy::object_id<tmam_metric_t, T, Policies>;
