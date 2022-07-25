#pragma once

#include <string>

#include <scorep/SCOREP_MetricTypes.h>
#include <scorep/plugin/plugin.hpp>

enum class tmam_metric_t {
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

/// get metric name for trace
std::string get_name(const tmam_metric_t metric);

template <typename T, typename Policies>
using tmam_metric_t_policy = scorep::plugin::policy::object_id<tmam_metric_t, T, Policies>;
