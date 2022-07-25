#include <metric.hpp>

#include <string>
#include <map>

std::string get_name(const tmam_metric_t metric) {
    const std::map<tmam_metric_t, std::string> name_by_metric = {
        {tmam_metric_t::slots, "slots"},
        {tmam_metric_t::bottleneck, "bottleneck"},
        {tmam_metric_t::l1_retiring, "l1-retiring"},
        {tmam_metric_t::l1_bad_speculation, "l1-bad-speculation"},
        {tmam_metric_t::l1_frontend_bound, "l1-frontend-bound"},
        {tmam_metric_t::l1_backend_bound, "l1-backend-bound"},
        {tmam_metric_t::l2_light_ops, "l2-light-ops"},
        {tmam_metric_t::l2_heavy_ops, "l2-light-ops"},
        {tmam_metric_t::l2_branch_misprediction, "l2-branch-misprediction"},
        {tmam_metric_t::l2_machine_clear, "l2-machine-clear"},
        {tmam_metric_t::l2_fetch_latency, "l2-fetch-latency"},
        {tmam_metric_t::l2_fetch_bandwidth, "l2-fetch-bandwidth"},
        {tmam_metric_t::l2_core_bound, "l2-core-bound"},
        {tmam_metric_t::l2_memory_bound, "l2-memory-bound"},
    };

    return "topdown-" + name_by_metric.at(metric);
}
