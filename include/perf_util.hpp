#pragma once

#include <cstdint>
#include <string>

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
    uint64_t nr = 0;
    uint64_t slots = 0;
    uint64_t retiring = 0;
    uint64_t bad_spec = 0;
    uint64_t fe_bound = 0;
    uint64_t be_bound = 0;
    uint64_t heavy_ops = 0;
    uint64_t br_mispredict = 0;
    uint64_t fetch_lat = 0;
    uint64_t mem_bound = 0;

    /// print to stderr
    void dump() const;

    /// generate csv (machine-readable) string
    std::string csv() const;

    /// get header for csv()
    static std::string csv_header();

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

/// component-wise addition
perf_tmam_data_t operator+(const perf_tmam_data_t& lhs, const perf_tmam_data_t& rhs);

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
     * internally use rdpmc, but emulate behavior of traditional perf
     * must be const, as rdpmc initialization is different from perf with counters
     */
    const bool use_rdpmc;

    /// store last tmam result for when using rdpmc
    perf_tmam_data_t rdpmc_last_result;

    /// pointer to memory region mapped to enable rdpmc
    void* rdpmc_mmap_ptr;

    /**
     * constructor
     *
     * opens & initializes TMAM perf events
     * @param use_rdpmc uses RDPMC when enabled, otherwise plain perf
     * @param pid thread to be monitored, current by default
     * @param cpu cpu to be monitored, any by default
     */
    perf_tmam_handle(bool use_rdpmc = false, pid_t pid = 0, int cpu = -1);

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
    perf_tmam_data_t read();

    /// trigger perf readout, but discard results
    void nullread();
};
