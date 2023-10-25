#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <string>
#include <mutex>

extern "C" {
    #include <unistd.h>
}

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

#include <scorep/SCOREP_MetricTypes.h>
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wvolatile"
#include <scorep/plugin/plugin.hpp>
#pragma GCC diagnostic pop

#include <metric.hpp>
#include <perf_util.hpp>

/// measurement state of a single thread
struct thread_state_t {
    /// perf handle
    perf_tmam_handle tmam_handle;

    /// latest sample
    perf_tmam_data_t sample_current;

    /// sample collected before latest
    perf_tmam_data_t sample_last;

    /// time at which sample_current has been collected
    std::chrono::steady_clock::time_point time_current;

    /// total number of collected samples
    uint64_t sample_cnt_total = 0;

    /// track when last data point for a particular metric has been recorded (metric != tmam sample)
    std::map<tmam_metric_t, std::chrono::steady_clock::time_point> last_metric_datapoint_timepoint_by_metric;

    /// constructor
    thread_state_t() : tmam_handle(false, 0, -1) {
        // nop, exists to initialize tmam_handle
    }
};
using thread_state_t = struct thread_state_t;

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

    /// mutex to access thread_state_by_thread (used for initialization only)
    std::mutex thread_state_by_thread_mutex;

    /// thread-local state
    std::map<tid_t, thread_state_t> thread_state_by_thread;

    /// minimum time between two measurements
    uint64_t delta_t_min_us = 500;

    /**
     * retrieve current sample for current thread if applicable
     *
     * Only if more than delta_t_min_us has passed since the last measurement will a sample be taken.
     *
     * @return if an actual update has been performed
     */
    void update_samples_this_thread() {
        const tid_t tid = get_current_tid();
        thread_state_t& ts = thread_state_by_thread.at(tid);
        
        // default: record new sample
        uint64_t passed_us = 1 + delta_t_min_us;
        auto now = std::chrono::steady_clock::now();

        if (0 < ts.sample_cnt_total) {
            // check if enough time passed
            passed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - ts.time_current).count();
        }

        if (passed_us < delta_t_min_us) {
            // not enough time passed -> skip update
            return;
        }

        // enough time passed, record new sample
        // (require check if already in map, if yes acquire required mutex)
        ts.time_current = now;
        if (0 < ts.sample_cnt_total){
            ts.sample_last = ts.sample_current;
        }
        ts.sample_current = ts.tmam_handle.read();
        ts.sample_cnt_total++;
    }

public:
    /// constructor
    topdown_plugin() {
        delta_t_min_us = std::stoull(scorep::environment_variable::get("INTERVAL_US",
                                                                       std::to_string(delta_t_min_us)));
    }

    void add_metric(const tmam_metric_t&) {
        // record given metric for *current thread*
        // note that there is no specific procedure for an individual metric,
        // all metrics are recorded by the same object
        // -> init tmam measurement *only* if not done already

        std::lock_guard lock(thread_state_by_thread_mutex);

        tid_t tid = get_current_tid();
        if (!thread_state_by_thread.contains(tid)) {
            // use emplace because handler may not be moved
            // use piecewise emplace, b/c handler has no constructor
            thread_state_by_thread.emplace(std::piecewise_construct,
                                           std::forward_as_tuple(tid),
                                           std::forward_as_tuple());
        }
    }

    template <class Proxy>
    bool get_optional_value(const tmam_metric_t& metric, Proxy& p) {
        tid_t tid = get_current_tid();
        thread_state_t& ts = thread_state_by_thread.at(tid);

        // 1. check if minimum since last sample (of this metric!!) passed

        // default: return data for metric
        uint64_t passed_us = 1 + delta_t_min_us;

        if (ts.last_metric_datapoint_timepoint_by_metric.contains(metric)) {
            auto last_tp = ts.last_metric_datapoint_timepoint_by_metric[metric];
            auto now = std::chrono::steady_clock::now();
            passed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tp).count();
        }

        if (passed_us < delta_t_min_us) {
            // not enough time passed since last metric readout, try again later
            return false;
        }

        // 2. (maybe) update samples
        // (will skip update if not enough time passed for)
        update_samples_this_thread();

        // 3. report value (if at least 2 samples are ready to compute deltas)
        if (ts.sample_cnt_total >= 2) {
            auto delta = ts.sample_current - ts.sample_last;

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
            ts.last_metric_datapoint_timepoint_by_metric[metric] = std::chrono::steady_clock::now();
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
