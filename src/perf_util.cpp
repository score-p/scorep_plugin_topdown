#include <perf_util.hpp>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <fstream>
#include <vector>

extern "C" {
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <linux/hw_breakpoint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <x86intrin.h>
}

perf_tmam_data_t operator-(const perf_tmam_data_t& lhs, const perf_tmam_data_t& rhs) {
    perf_tmam_data_t result = lhs;
    result.nr = lhs.nr - rhs.nr;
    result.slots = lhs.slots - rhs.slots;
    result.retiring = lhs.retiring - rhs.retiring;
    result.bad_spec = lhs.bad_spec - rhs.bad_spec;
    result.fe_bound = lhs.fe_bound - rhs.fe_bound;
    result.be_bound = lhs.be_bound - rhs.be_bound;
    result.heavy_ops = lhs.heavy_ops - rhs.heavy_ops;
    result.br_mispredict = lhs.br_mispredict - rhs.br_mispredict;
    result.fetch_lat = lhs.fetch_lat - rhs.fetch_lat;
    result.mem_bound = lhs.mem_bound - rhs.mem_bound;

    return result;
}

perf_tmam_data_t operator+(const perf_tmam_data_t& lhs, const perf_tmam_data_t& rhs) {
    perf_tmam_data_t result = lhs;
    result.nr = lhs.nr + rhs.nr;
    result.slots = lhs.slots + rhs.slots;
    result.retiring = lhs.retiring + rhs.retiring;
    result.bad_spec = lhs.bad_spec + rhs.bad_spec;
    result.fe_bound = lhs.fe_bound + rhs.fe_bound;
    result.be_bound = lhs.be_bound + rhs.be_bound;
    result.heavy_ops = lhs.heavy_ops + rhs.heavy_ops;
    result.br_mispredict = lhs.br_mispredict + rhs.br_mispredict;
    result.fetch_lat = lhs.fetch_lat + rhs.fetch_lat;
    result.mem_bound = lhs.mem_bound + rhs.mem_bound;

    return result;
}

void perf_tmam_data_t::dump() const{
    using std::cerr;
    using std::endl;

    cerr << "      RET         BAD SPEC      FE           BE"
         << "         Total Slots: " << std::setw(14) << slots << endl;
    cerr << std::setprecision(0) << std::fixed
         << "      " << std::setw(2) << 100.0 * retiring / slots
         << "%           " << std::setw(2) << 100.0 * bad_spec / slots
         << "%         " << std::setw(2) << 100.0 * fe_bound / slots
         << "%          " << std::setw(2) << 100.0 * be_bound / slots
         << "%" << endl;
    cerr << "  LIGHT HEAVY   MISPR CLEAR   LAT BANDW   CORE MEM" << endl;
    cerr << std::setprecision(0) << std::fixed
         << "   " << std::setw(2) << 100.0 * (retiring - heavy_ops) / slots
         << "%    " << std::setw(2) << 100.0 * heavy_ops / slots
         << "%     " << std::setw(2) << 100.0 * br_mispredict / slots
         << "%   " << std::setw(2) << 100.0 * (bad_spec - br_mispredict) / slots
         << "%   " << std::setw(2) << 100.0 * fetch_lat / slots
         << "%  " << std::setw(2) << 100.0 * (fe_bound - fetch_lat) / slots
         << "%     " << std::setw(2) << 100.0 * (be_bound - mem_bound) / slots
         << "% " << std::setw(2) << 100.0 * mem_bound / slots
         << "%" << endl;
}

std::string perf_tmam_data_t::csv() const {
    std::vector<uint64_t> output_unscaled = {
        // l1
        retiring,
        bad_spec,
        fe_bound,
        be_bound,

        // l2
        retiring - heavy_ops,
        heavy_ops,
        br_mispredict,
        bad_spec - br_mispredict,
        fetch_lat,
        fe_bound - fetch_lat,
        be_bound - mem_bound,
        mem_bound,
    };

    std::string csv;

    // total slots first
    csv = std::to_string(slots) + ";";

    // then all categories (scaled)
    for (const auto unscaled : output_unscaled) {
        csv += std::to_string(static_cast<double>(unscaled) / static_cast<double>(slots)) + ";";
    }

    return csv;
}

std::string perf_tmam_data_t::csv_header() {
    return "slots;retiring;bad_spec;fe_bound;be_bound;light_ops;heavy_ops;br_mispredict;machine_clear;fetch_lat;fetch_bandwidth;core_bound;mem_bound;";
}


int checked_perf_open(struct perf_event_attr* attr_ptr, pid_t pid, int cpu, int group, int flags) {
    auto perf_fd = syscall(SYS_perf_event_open,
                           attr_ptr,
                           pid, // given process
                           cpu, // given cpu (-1 for any)
                           group, // fd of group lead (or -1 for separate group)
                           flags); // given flags (0 if none)
    if (perf_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "call to perf_event_open failed");
    }

    return perf_fd;
}

perf_tmam_data_t perf_tmam_data_t::read_from_perf(int perf_leader_fd) {
    perf_tmam_data_t data;
    if (sizeof(perf_tmam_data_t) != read(perf_leader_fd, &data, sizeof(perf_tmam_data_t))) {
        throw std::runtime_error("perf read failed");
    }
    return data;
}

unsigned int perf_tmam_handle::get_pmu_type() {
    const std::string fname = "/sys/devices/cpu_core/type";
    unsigned int type;
    std::ifstream typefile(fname);

    if (!typefile.is_open()) {
        throw std::runtime_error("could not open " + fname);
    }

    typefile >> type;

    if (0 == type) {
        throw std::system_error(EINVAL, std::generic_category(), "perf PMU type must not be zero");
    }

    return type;
}

perf_tmam_handle::perf_tmam_handle(bool use_rdpmc, pid_t pid, int cpu) : use_rdpmc(use_rdpmc) {
    // phase 1: open leader


    // Note that "config" is using hardcoded values throughout.
    // This is because a) the documentation notes these numbers,
    // so they are assumed to be stable and b) they are hardcoded in the kernel as well.

    // Also, the perf_event_attr struct is huged and designed to no be specified for many fields.
    // Therefore the pragma to supress the warnings.

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    struct perf_event_attr leader_perf_attr = {
        .type = get_pmu_type(),
        .size = sizeof(struct perf_event_attr),
        .config = 0x400,
        .read_format = PERF_FORMAT_GROUP,
        .disabled = 1,
    };
#pragma GCC diagnostic pop 

    fd_leader = checked_perf_open(&leader_perf_attr,
                                  pid, // given thread, (0 == current by default)
                                  cpu, // given cpu (-1 == any by default)
                                  -1, // in separate group
                                  0ul); // no flags

    // phase 2: open other events
    auto get_tmam_perf_fd = [&](uint64_t config) {
        // ignore warnings, see above for explanation
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        struct perf_event_attr perf_attr = {
            .type = get_pmu_type(),
            .size = sizeof(struct perf_event_attr),
            .config = config,
            .read_format = PERF_FORMAT_GROUP,
            .disabled = 0,
        };
#pragma GCC diagnostic pop 

        return  checked_perf_open(&perf_attr,
                                  pid,
                                  cpu,
                                  fd_leader, // grouped with leader
                                  0ul); // no flags
    };

    if (!use_rdpmc) {
        // perf with counters: open every counter individually
        fd_retiring = get_tmam_perf_fd(0x8000);
        fd_bad_spec = get_tmam_perf_fd(0x8100);
        fd_fe_bound = get_tmam_perf_fd(0x8200);
        fd_be_bound = get_tmam_perf_fd(0x8300);

        fd_heavy_ops = get_tmam_perf_fd(0x8400);
        fd_br_mispredict = get_tmam_perf_fd(0x8500);
        fd_fetch_lat = get_tmam_perf_fd(0x8600);
        fd_mem_bound = get_tmam_perf_fd(0x8700);
    } else {
        // rdpmc: only enable one counter, then use mmap
        fd_retiring = get_tmam_perf_fd(0x8000);
        rdpmc_mmap_ptr = mmap(0, // no pre-defined code region
                              getpagesize(), // size of one page (duh)
                              PROT_READ, // read-only
                              MAP_SHARED, // share among all processes of the underlying file
                              fd_retiring, // file to map (perf events)
                              0); // no offset
        if (MAP_FAILED == rdpmc_mmap_ptr) {
            throw std::system_error(errno, std::generic_category(), "memory mapping for rdpmc failed");
        }

        // initial reset
        ioctl(fd_leader, PERF_EVENT_IOC_RESET, 0);
    }

    // enable counting
    ioctl(fd_leader, PERF_EVENT_IOC_ENABLE);
}

perf_tmam_handle::~perf_tmam_handle() {
    close(fd_leader);
    close(fd_retiring);

    if (!use_rdpmc) {
        // these FDs have only been opened when using vanilla perf (==NOT using rdpmc)
        close(fd_bad_spec);
        close(fd_fe_bound);
        close(fd_be_bound);
        close(fd_heavy_ops);
        close(fd_br_mispredict);
        close(fd_fetch_lat);
        close(fd_mem_bound);
    } else {
        // unmap mapped memory region, mapped to support rdpmc
        munmap(rdpmc_mmap_ptr, getpagesize());
    }
}

perf_tmam_data_t perf_tmam_handle::read() {
    if (!use_rdpmc) {
        // traditional perf with counters is trivial
        return perf_tmam_data_t::read_from_perf(fd_leader);
    }

    // see RDPMC instruction reference
    constexpr uint32_t rdpmc_bitmask_fixed = 1ul << 30;
    constexpr uint32_t rdpmc_bitmask_slots = 3ul;
    constexpr uint32_t rdpmc_bitmask_tmam = 1ul << 29;

    // with rdpmc: emulate perf behavior
    perf_tmam_data_t result;
    result.nr = 9;
    uint64_t new_slots = _rdpmc(rdpmc_bitmask_fixed | rdpmc_bitmask_slots);
    uint64_t tmam_raw = _rdpmc(rdpmc_bitmask_tmam);

    // split & scale according to tmam results
    //result.slots = new_slots - rdpmc_last_result.slots;
    result.slots = new_slots;
    const auto split_and_scale = [=](int offset) {
        uint64_t bitmask = 0xffull << offset;
        uint64_t extracted = (tmam_raw & bitmask) >> offset;
        // scale to number of slots
        return extracted * result.slots / 0xffull;
    };

    result.retiring = split_and_scale(0);
    result.bad_spec = split_and_scale(8);
    result.fe_bound = split_and_scale(16);
    result.be_bound = split_and_scale(24);
    result.heavy_ops = split_and_scale(32);
    result.br_mispredict = split_and_scale(40);
    result.fetch_lat = split_and_scale(48);
    result.mem_bound = split_and_scale(56);

    // accumulate with next result
    rdpmc_last_result = rdpmc_last_result + result;

    // reset next period
    ioctl(fd_leader, PERF_EVENT_IOC_RESET, 0);
    return rdpmc_last_result;
}

void perf_tmam_handle::nullread() {
    if (42 == read().slots) {
        std::cerr << " \b";
    }
}
