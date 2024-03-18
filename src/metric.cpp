#include <metric.hpp>

#include <string>
#include <map>
#include <stdexcept>

const std::vector<tmam_metric_t> tmam_metric_t::all = {
    tmam_metric_t(tmam_metric_category::slots),
    tmam_metric_t(tmam_metric_category::l1_bottleneck),
    tmam_metric_t(tmam_metric_category::l2_bottleneck),
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
        {tmam_metric_category::l1_bottleneck, "l1-bottleneck"},
        {tmam_metric_category::l2_bottleneck, "l2-bottleneck"},
        {tmam_metric_category::l1_retiring, "l1-retiring"},
        {tmam_metric_category::l1_bad_speculation, "l1-bad-speculation"},
        {tmam_metric_category::l1_frontend_bound, "l1-frontend-bound"},
        {tmam_metric_category::l1_backend_bound, "l1-backend-bound"},
        {tmam_metric_category::l2_light_ops, "l2-light-ops"},
        {tmam_metric_category::l2_heavy_ops, "l2-heavy-ops"},
        {tmam_metric_category::l2_branch_misprediction, "l2-branch-misprediction"},
        {tmam_metric_category::l2_machine_clear, "l2-machine-clear"},
        {tmam_metric_category::l2_fetch_latency, "l2-fetch-latency"},
        {tmam_metric_category::l2_fetch_bandwidth, "l2-fetch-bandwidth"},
        {tmam_metric_category::l2_core_bound, "l2-core-bound"},
        {tmam_metric_category::l2_memory_bound, "l2-memory-bound"},
    };

    return "topdown-" + name_by_metric.at(category);
}

std::string tmam_metric_t::get_description() const {
    const std::map<tmam_metric_category, std::string> description_by_metric = {
        {
            tmam_metric_category::slots,
            "number of uOP issue slots"
        },
        {
            tmam_metric_category::l1_bottleneck,
            "level 1 category with the most alotted time (retiring ignored)"
        },
        {
            tmam_metric_category::l2_bottleneck,
            "level 2 category with the most alotted time (light/heavy ops ignored)"
        },
        {
            tmam_metric_category::l1_retiring,
            "retiring (commited, 'good') uOPs"
        },
        {
            tmam_metric_category::l1_bad_speculation,
            "bad speculation"
        },
        {
            tmam_metric_category::l1_frontend_bound,
            "frontend bound (not enough uOPs provided to backend)"
        },
        {
            tmam_metric_category::l1_backend_bound,
            "backend bound (not enough uOPs consumed by backend)"
        },
        {
            tmam_metric_category::l2_light_ops,
            "# uOPs that originate from light OPs (translate to 1 uOP)"
        },
        {
            tmam_metric_category::l2_heavy_ops,
            "# uOPs that originate from heavy OPs (translate to >=2 uOPs)"
        },
        {
            tmam_metric_category::l2_branch_misprediction,
            "branch misprediction"
        },
        {
            tmam_metric_category::l2_machine_clear,
            "machine clear"
        },
        {
            tmam_metric_category::l2_fetch_latency,
            "fetch latency"
        },
        {
            tmam_metric_category::l2_fetch_bandwidth,
            "fetch bandwidth"
        },
        {
            tmam_metric_category::l2_core_bound,
            "bound by core resources (some form of execution units)"
        },
        {
            tmam_metric_category::l2_memory_bound,
            "bound by memory (includes caches, external mem etc.)"
        },
    };

    return description_by_metric.at(category);
}

scorep::plugin::metric_property tmam_metric_t::get_metric_property() const {
    scorep::plugin::metric_property mp(get_name(), get_description(), "fraction");

    mp.mode = SCOREP_METRIC_MODE_ABSOLUTE_LAST;

    // type: all are fractions (of 1), except slots (#) and bottleneck (category enum)
    if (tmam_metric_category::l1_bottleneck == category ||
        tmam_metric_category::l2_bottleneck == category ||
        tmam_metric_category::slots == category) {
        mp.type = SCOREP_METRIC_VALUE_UINT64;
    } else {
        mp.type = SCOREP_METRIC_VALUE_DOUBLE;
    }

    // override if category -> different unit
    if (tmam_metric_category::l1_bottleneck == category ||
        tmam_metric_category::l2_bottleneck == category) {
        mp.unit = "TMAM category";
    } else if (tmam_metric_category::slots == category) {
        mp.unit = "#";
    }

    return mp;
}

bool operator<(const tmam_metric_t& lhs, const tmam_metric_t& rhs) {
    return lhs.category < rhs.category;
}


uint64_t tmam_metric_t::extract_tmam_field(const perf_tmam_data_t& tmam) const {
    switch(category) {
    case tmam_metric_category::slots:
        return tmam.slots;
    case tmam_metric_category::l1_bottleneck:
        return static_cast<uint64_t>(get_l1_bottleneck(tmam));
    case tmam_metric_category::l2_bottleneck:
        return static_cast<uint64_t>(get_l2_bottleneck(tmam));
    case tmam_metric_category::l1_retiring:
        return tmam.retiring;
    case tmam_metric_category::l1_bad_speculation:
        return tmam.bad_spec;
    case tmam_metric_category::l1_frontend_bound:
        return tmam.fe_bound;
    case tmam_metric_category::l1_backend_bound:
        return tmam.be_bound;
    case tmam_metric_category::l2_light_ops:
        return tmam.retiring - tmam.heavy_ops;
    case tmam_metric_category::l2_heavy_ops:
        return tmam.heavy_ops;
    case tmam_metric_category::l2_branch_misprediction:
        return tmam.br_mispredict;
    case tmam_metric_category::l2_machine_clear:
        return tmam.bad_spec - tmam.br_mispredict;
    case tmam_metric_category::l2_fetch_latency:
        return tmam.fetch_lat;
    case tmam_metric_category::l2_fetch_bandwidth:
        return tmam.fe_bound - tmam.fetch_lat;
    case tmam_metric_category::l2_core_bound:
        return tmam.be_bound - tmam.mem_bound;
    case tmam_metric_category::l2_memory_bound:
        return tmam.mem_bound;
    }

    throw std::runtime_error("unkown tmam category encountered: " + std::to_string(static_cast<uint64_t>(category)));
}

tmam_metric_category tmam_metric_t::get_l2_bottleneck(const perf_tmam_data_t& tmam) {
    const std::vector<tmam_metric_category> lvl2_categories = {
        // ignore retiring, this is not a bottleneck!
        // (to be fair it can be when not using vector instructions etc., but it is typically not)
        tmam_metric_category::l2_branch_misprediction,
        tmam_metric_category::l2_machine_clear,
        tmam_metric_category::l2_fetch_latency,
        tmam_metric_category::l2_fetch_bandwidth,
        tmam_metric_category::l2_core_bound,
        tmam_metric_category::l2_memory_bound,
    };

    uint64_t top_slots = 0;
    tmam_metric_category top_category = tmam_metric_category::l2_light_ops;

    for (const auto category : lvl2_categories) {
        uint64_t slots = tmam_metric_t(category).extract_tmam_field(tmam);
        if (slots > top_slots) {
            top_slots = slots;
            top_category = category;
        }
    }

    return top_category;
}

tmam_metric_category tmam_metric_t::get_l1_bottleneck(const perf_tmam_data_t& tmam) {
    const std::vector<tmam_metric_category> lvl2_categories = {
        // ignore retiring, this is not a bottleneck!
        // (to be fair it can be when not using vector instructions etc., but it is typically not)
        tmam_metric_category::l1_bad_speculation,
        tmam_metric_category::l1_frontend_bound,
        tmam_metric_category::l1_backend_bound,
    };

    uint64_t top_slots = 0;
    tmam_metric_category top_category = tmam_metric_category::l1_retiring;

    for (const auto category : lvl2_categories) {
        uint64_t slots = tmam_metric_t(category).extract_tmam_field(tmam);
        if (slots > top_slots) {
            top_slots = slots;
            top_category = category;
        }
    }

    return top_category;
}

tmam_metric_t::tmam_metric_t (tmam_metric_category category) : category(category){
    // nop
}
