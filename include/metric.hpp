#pragma once

#include <string>

#include <scorep/SCOREP_MetricTypes.h>
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wvolatile"
#include <scorep/plugin/plugin.hpp>
#pragma GCC diagnostic pop

#include <perf_util.hpp>

/**
 * category of tmam result.
 * One scalar which can be extracted from a tmam result
 *
 * note that the numbers assigned here are used in exported traces,
 * so **DO NOT REASSIGN NUMBERS*.
 */
enum class tmam_metric_category : uint64_t {
    slots = 1ull << 40,
    l1_bottleneck = (1ull << 40) + 1,
    l2_bottleneck = (1ull << 40) + 2,

    // start count from 0 such that traces have "nice" numbers
    // (note: these are in the order as mentioned in the optimization manual figure)
    l1_retiring = 0,
    l1_bad_speculation = 1,
    l1_frontend_bound = 2,
    l1_backend_bound = 3,
    l2_light_ops = 4,
    l2_heavy_ops = 5,
    l2_branch_misprediction = 6,
    l2_machine_clear = 7,
    l2_fetch_latency = 8,
    l2_fetch_bandwidth = 9,
    l2_core_bound = 10,
    l2_memory_bound = 11,
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

    tmam_metric_t (tmam_metric_category category);

    /// list of all supported metrics
    static const std::vector<tmam_metric_t> all;
    
    /// get metric name for trace
    std::string get_name() const;

    /// get metric description
    std::string get_description() const;

    /// retrieve scorep metric type
    scorep::plugin::metric_property get_metric_property() const;

    /**
     * extract category from given tmam results
     *
     * @param tmam results to examine
     * @return field from tmam given by this->category
     */
    uint64_t extract_tmam_field(const perf_tmam_data_t& tmam) const;

    /**
     * extract l2 category with the most alotted slots, retiring (light/heavy ops) are ignored!
     * @param tmam results to examine
     * @return l2 category with highest count
     */
    static tmam_metric_category get_l2_bottleneck(const perf_tmam_data_t& tmam);

    /**
     * extract l1 category with the most alotted slots, retiring are ignored!
     * @param tmam results to examine
     * @return l1 category with highest count
     */
    static tmam_metric_category get_l1_bottleneck(const perf_tmam_data_t& tmam);
};

bool operator<(const tmam_metric_t& lhs, const tmam_metric_t& rhs);

template <typename T, typename Policies>
using tmam_metric_t_policy = scorep::plugin::policy::object_id<tmam_metric_t, T, Policies>;
