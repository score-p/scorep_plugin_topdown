#include <metric.hpp>

#include <string>
#include <map>

const std::vector<tmam_metric_t> tmam_metric_t::all = {
    tmam_metric_t(tmam_metric_category::slots),
    tmam_metric_t(tmam_metric_category::bottleneck),
    tmam_metric_t(tmam_metric_category::l1_retiring),
    tmam_metric_t(tmam_metric_category::l1_bad_speculation),
    tmam_metric_t(tmam_metric_category::l1_frontend_bound),
    tmam_metric_t(tmam_metric_category::l1_backend_bound),
    tmam_metric_t(tmam_metric_category::l2_light_ops),
    tmam_metric_t(tmam_metric_category::l2_heavy_ops),
    tmam_metric_t(tmam_metric_category::l2_branch_misprediction),
    tmam_metric_t(tmam_metric_category::l2_machine_clear),
    tmam_metric_t(tmam_metric_category::l2_fetch_latency),
    tmam_metric_t(tmam_metric_category::l2_fetch_bandwidth),
    tmam_metric_t(tmam_metric_category::l2_core_bound),
    tmam_metric_t(tmam_metric_category::l2_memory_bound),
};

std::string tmam_metric_t::get_name() const {
    const std::map<tmam_metric_category, std::string> name_by_metric = {
        {tmam_metric_category::slots, "slots"},
        {tmam_metric_category::bottleneck, "bottleneck"},
        {tmam_metric_category::l1_retiring, "l1-retiring"},
        {tmam_metric_category::l1_bad_speculation, "l1-bad-speculation"},
        {tmam_metric_category::l1_frontend_bound, "l1-frontend-bound"},
        {tmam_metric_category::l1_backend_bound, "l1-backend-bound"},
        {tmam_metric_category::l2_light_ops, "l2-light-ops"},
        {tmam_metric_category::l2_heavy_ops, "l2-light-ops"},
        {tmam_metric_category::l2_branch_misprediction, "l2-branch-misprediction"},
        {tmam_metric_category::l2_machine_clear, "l2-machine-clear"},
        {tmam_metric_category::l2_fetch_latency, "l2-fetch-latency"},
        {tmam_metric_category::l2_fetch_bandwidth, "l2-fetch-bandwidth"},
        {tmam_metric_category::l2_core_bound, "l2-core-bound"},
        {tmam_metric_category::l2_memory_bound, "l2-memory-bound"},
    };

    return "topdown-" + name_by_metric.at(category);
}
