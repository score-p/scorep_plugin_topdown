#pragma once

#include <cstdint>

extern "C" {
#include <sys/types.h>
#include <linux/perf_event.h>
}

/**
 * holds all data associated with one perf TMAM readout
 *
 * Using `.read_format = PERF_FORMAT_GROUP` when opening perf means one can directly read into this struct with a single read.
 *
 * Note that the data are counters of events i.e., they are constantly accumulating (growing).
 * Fractions/Percentages are based on the slots member.
 */
struct perf_tmam_data_t {
    /// nr. of contained events, required for perf support
    uint64_t nr;
    uint64_t slots;
    uint64_t retiring;
    uint64_t bad_spec;
    uint64_t fe_bound;
    uint64_t be_bound;
    uint64_t heavy_ops;
    uint64_t br_mispredict;
    uint64_t fetch_lat;
    uint64_t mem_bound;

    /// print to stderr
    void dump() const;

    /**
     * read TMAM data from perf
     *
     * typical way to create a perf_tmam_data_t object.
     * @param perf_leader_fd file descriptor of perf TMAM group
     * @return data as returned by perf
     */
    static perf_tmam_data_t read_from_perf(int perf_leader_fd);
};
typedef struct perf_tmam_data_t perf_tmam_data_t;

/// component-wise subtraction
perf_tmam_data_t operator-(const perf_tmam_data_t& lhs, const perf_tmam_data_t& rhs);

/// call syscall w/ error-checking
int checked_perf_open(struct perf_event_attr* attr_ptr, pid_t pid, int cpu, int group, int flags);

/**
 * RAII perf TMAM events
 *
 * Opens the TMAM events (for current process, any CPU) on construction and closes them on desctruction.
 */
class perf_tmam_handle {
public:
    /// fd of leader (slots)
    int fd_leader;

    int fd_retiring;
    int fd_bad_spec;
    int fd_fe_bound;
    int fd_be_bound;

    int fd_heavy_ops;
    int fd_br_mispredict;
    int fd_fetch_lat;
    int fd_mem_bound;

    /**
     * constructor
     *
     * opens & initializes TMAM perf events
     */
    perf_tmam_handle();

    // delete move/copy constructors, which screws with RAII and file handle closing
    perf_tmam_handle (const perf_tmam_handle&) = delete;
    perf_tmam_handle& operator= (const perf_tmam_handle&) = delete;

    /**
     * destructor
     *
     * closes opened TMAM events
     */
    virtual ~perf_tmam_handle();

    /// read TMAM results
    perf_tmam_data_t read() const;

    /// trigger perf readout, but discard results
    void nullread() const;

    /// helper: retrieve perf event type for P-core PMU
    static unsigned int get_pmu_type();
};
