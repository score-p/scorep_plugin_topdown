#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <chrono>

extern "C" {
    #include <unistd.h>
}

#include <scorep/SCOREP_MetricTypes.h>
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wvolatile"
#include <scorep/plugin/plugin.hpp>
#pragma GCC diagnostic pop

#include <metric.hpp>
#include <perf_util.hpp>

class topdown_plugin :
    public scorep::plugin::base<topdown_plugin,
                                scorep::plugin::policy::sync,
                                scorep::plugin::policy::per_thread,
                                scorep::plugin::policy::scorep_clock,
                                tmam_metric_t_policy> {
private:
    // thread id type -> corresponding to pid_t
    using tid_t = pid_t;

    /// retrieve id of current thread
    tid_t get_current_tid() const {
        return gettid();
    }

    /// handle to TMAM perf wrapper (per thread)
    std::map<tid_t, perf_tmam_handle> tmam_handle_by_thread;

    /// minimum time between two measurements
    uint64_t delta_t_min_us = 10000;

    /// track when last measurement was taken
    std::map<tid_t, std::chrono::steady_clock::time_point> sample_current_timepoint_by_thread;

    /// track when last data point for a particular metric has been recorded (metric != tmam sample)
    std::map<tid_t, std::map<tmam_metric_t, std::chrono::steady_clock::time_point>> last_metric_datapoint_timepoint_by_metric_by_thread;

    /// most recent sample, stored to re-use same sample across multiple metrics
    std::map<tid_t, perf_tmam_data_t> sample_current_by_thread;

    /// last sample, stored to compute delta to sample_current_by_thread
    std::map<tid_t, perf_tmam_data_t> sample_last_by_thread;

    /**
     * retrieve current sample for current thread if applicable
     *
     * Only if more than delta_t_min_us has passed since the last measurement will a sample be taken.
     *
     * @return if an actual update has been performed
     */
    void update_samples_this_thread() {
        tid_t tid = get_current_tid();
        // default: record new sample
        uint64_t passed_us = 1 + delta_t_min_us;
        auto now = std::chrono::steady_clock::now();

        if (sample_current_timepoint_by_thread.contains(tid)) {
            // check if enough time passed
            passed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - sample_current_timepoint_by_thread[tid]).count();
        }

        if (passed_us < delta_t_min_us) {
            // not enough time passed -> skip update
            return;
        }

        // enough time passed, record new sample
        sample_current_timepoint_by_thread[tid] = now;
        if (sample_current_by_thread.contains(tid)) {
            sample_last_by_thread[tid] = sample_current_by_thread[tid];
        }
        sample_current_by_thread[tid] = tmam_handle_by_thread[tid].read();
    }

public:
    void add_metric(const tmam_metric_t&) {
        // record given metric for *current thread*
        // note that there is no specific procedure for an individual metric,
        // all metrics are recorded by the same object
        // -> init tmam measurement if not done already
        tid_t tid = get_current_tid();
        if (!tmam_handle_by_thread.contains(tid)) {
            // use emplace because handler may not be moved
            // use piecewise emplace, b/c handler has no constructor
            tmam_handle_by_thread.emplace(std::piecewise_construct,
                                          std::forward_as_tuple(tid),
                                          std::forward_as_tuple());
        }
    }

    template <class Proxy>
    bool get_optional_value(const tmam_metric_t& metric, Proxy& p) {
        // 1. check if minimum since last sample (of this metric!!) passed

        tid_t tid = get_current_tid();
        // default: record new sample
        uint64_t passed_us = 1 + delta_t_min_us;

        if (last_metric_datapoint_timepoint_by_metric_by_thread[tid].contains(metric)) {
            auto last_tp = last_metric_datapoint_timepoint_by_metric_by_thread[tid][metric];
            auto now = std::chrono::steady_clock::now();
            passed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tp).count();
        }

        if (passed_us < delta_t_min_us) {
            // not enough time passed, try again later
            return false;
        }

        // 2. (maybe) update samples
        // (will skip update if not enough time passed for)
        update_samples_this_thread();

        // 3. report value (if all values ready)
        if (sample_current_by_thread.contains(tid) && sample_last_by_thread.contains(tid)) {
            auto current = sample_current_by_thread[tid];
            auto last = sample_last_by_thread[tid];
            auto delta = current - last;

            uint64_t value_raw = metric.extract_tmam_field(delta);

            if (tmam_metric_category::slots == metric.category ||
                tmam_metric_category::l1_bottleneck == metric.category ||
                tmam_metric_category::l2_bottleneck == metric.category) {
                // slots & bottlenecks are reported as-is
                p.write(value_raw);
            } else {
                // all other metrics are reported as fractions [0,1]
                p.write(static_cast<double>(value_raw) / static_cast<double>(delta.slots));
            }

            // 4. update last recorded time
            // note: it is critical that we update the time *after* update_samples_this_read()
            // when using this order, if the last metric recording is >= delta_t_min_us away,
            // the last readout of the perf handler will be too
            // (when assigning the other way around, this must not necessarily be true)
            last_metric_datapoint_timepoint_by_metric_by_thread[tid][metric] = std::chrono::steady_clock::now();
            return true;
        }

        // not all values ready -> no value to be reported
        return false;
    }

    std::vector<scorep::plugin::metric_property> get_metric_properties(const std::string& pattern) {
        if ("*" != pattern) {
            throw std::runtime_error("pattern must be '*'");
        }

        std::vector<scorep::plugin::metric_property> result;
        for (const auto& metric : tmam_metric_t::all) {
            make_handle(metric.get_name(), metric);
            result.push_back(metric.get_metric_property());
        }

        return result;
    }
};
