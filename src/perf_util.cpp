#include <perf_util.hpp>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <fstream>

extern "C" {
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <linux/hw_breakpoint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
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

perf_tmam_handle::perf_tmam_handle() {
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
                                  0, // this process
                                  -1, // any cpu
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
                                  0, // this process
                                  -1, // any cpu
                                  fd_leader, // grouped with leader
                                  0ul); // no flags
    };

    fd_retiring = get_tmam_perf_fd(0x8000);
    fd_bad_spec = get_tmam_perf_fd(0x8100);
    fd_fe_bound = get_tmam_perf_fd(0x8200);
    fd_be_bound = get_tmam_perf_fd(0x8300);

    fd_heavy_ops = get_tmam_perf_fd(0x8400);
    fd_br_mispredict = get_tmam_perf_fd(0x8500);
    fd_fetch_lat = get_tmam_perf_fd(0x8600);
    fd_mem_bound = get_tmam_perf_fd(0x8700);

    // enable counting
    ioctl(fd_leader, PERF_EVENT_IOC_ENABLE);
}

perf_tmam_handle::~perf_tmam_handle() {
    close(fd_leader);
    close(fd_retiring);
    close(fd_bad_spec);
    close(fd_fe_bound);
    close(fd_be_bound);
    close(fd_heavy_ops);
    close(fd_br_mispredict);
    close(fd_fetch_lat);
    close(fd_mem_bound);
}

perf_tmam_data_t perf_tmam_handle::read() const {
    return perf_tmam_data_t::read_from_perf(fd_leader);
}

void perf_tmam_handle::nullread() const {
    if (42 == read().slots) {
        std::cerr << " \b";
    }
}
