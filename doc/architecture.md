# Architecture
This file sketches the rough architecture of this plugin.
Overall it is not too much code, in case of problems/unclear sections please just contact me/open an issue.

- `lib/scorep_plugin_cxx_wrapper`: [scorep plugin c++ wrapper](https://github.com/score-p/scorep_plugin_cxx_wrapper/), makes development easier

- `src/metric.cpp`, `include/metric.hpp`:
  Define TMAM category enum `tmam_metric_category`, which not only includes the bare 12 categories but also derived/auxilliary metrics as *number of slots*, *bottleneck of level 1/2*.

  Define `tmam_metric_t`, which represents **one metric** that will be recorded into the OTF2 trace.
  Furthermore, translation functions to compute the metric from perf samples are provided.
- `include/perf_util.hpp`, `src/perf_util.cpp`:
  Define `perf_tmam_data_t` which holds all data associated to **one TMAM measurement**.
  It is structured s.t. that one perf read reads all counters at once.
  Note that the `perf_tmam_data_t` contains **number of uOP issue slots** (not fractions),
  which continously accumulate when recorded from perf.
  Accordingly, before reporting for short versions use `operator-` to compute the difference between two measurements.
  
  Define `perf_tmam_handle` as RAII perf handle (similar to `std::fstream` etc.).
  Only works on *current thread*, query perf by calling `read()` which creates one `perf_tmam_data_t` instance (using accumulated counters!).
- `src/plugin.cpp`, `include/plugin.hpp`:
  Actual plugin source.

  Manage thread-perf handler-OTF2 metric association,
  translate perf-reported accumulator to region-exclusive fractions between 0 and 1,
  handle minimum interval between samples

