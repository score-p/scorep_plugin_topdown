# Top-down Microarchitecture Analysis Method Score-P Plugin
This plugin adds [Top-down Microarchitecture Analysis Method](https://ieeexplore.ieee.org/document/6844459) (TMAM) support into Score-P traces.

Note that it only supports cores using the Golden Cove microarchitecture (at least Alder Lake P-cores/Sapphire Rapids CPUs).

Further Reading:
[Slidedeck by Intel](https://pdfs.semanticscholar.org/b5e0/1ab1baa6640a39edfa06d556fabd882cdf64.pdf) |
[Initial Publication](https://ieeexplore.ieee.org/document/6844459) |
[Optimization Manual](https://cdrdv2.intel.com/v1/dl/getContent/671488), Appendix B.1 |
[vTune cookbook](https://www.intel.com/content/www/us/en/develop/documentation/vtune-cookbook/top/methodologies/top-down-microarchitecture-analysis-method.html) |
[perf doc](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/tools/perf/Documentation/topdown.txt)

## Reported Values
For **every thread** the following metrics are recorded:

- level 1 metrics (`topdown-l1-*`, fraction [0,1]): retiring, bad speculation, backend bound, frontend bound
- level 2 metrics (`topdown-l2-*`, fraction [0,1]): light ops, heavy ops, branch mispredictions, machine clears, fetch latency, fetch bandwidth, core bound, memory bound
- slots (`topdown-slots`, count): number of uOP issue slots in measured section; can be used to scale fractions to number of uOP issue slots
- bottlenecks (`topdown-l1-bottleneck` and `topdown-l2-bottlneck`, magic numbers): category of level 1/2 which has the highest fraction (got the most uOP issue slots) *without* retiring categories (l1: retiring, l2: light & heavy ops)

![example trace of BT](https://user-images.githubusercontent.com/80697868/181500902-21241fc2-9196-45c2-a8fe-1fd89d7fd072.png)
([download full trace](https://github.com/score-p/scorep_plugin_topdown/files/9209217/scorep-20220728_1328_2452601511261376.tar.gz))

The `topdown-l{1,2}-bottleneck` metrics encode the categories as follows ([defined here](./include/metric.hpp)):

| Number in Trace | Level | Name                 |
|-----------------|-------|----------------------|
| 0               | 1     | retiring (unused)    |
| 1               | 1     | bad speculation      |
| 2               | 1     | frontend bound       |
| 3               | 1     | backend bound        |
| 4               | 2     | light ops (unused)   |
| 5               | 2     | heavy ops (unused)   |
| 6               | 2     | branch misprediction |
| 7               | 2     | machine clear        |
| 8               | 2     | fetch latency        |
| 9               | 2     | fetch bandwidth      |
| 10              | 2     | core bound           |
| 11              | 2     | memory bound         |

## Requirements
A CPU that supports the extended Top-down microarchitecture results register.
At the time of writing, this is only available on Alder Lake and will be featured in the upcoming Sapphire Rapids CPUs.

## Configuration
This plugin is configured via environment variables as follows:

- `SCOREP_METRIC_PLUGINS=topdown_plugin` (required): load plugin into scorep
- `SCOREP_METRIC_TOPDOWN_PLUGIN='*'` (required): enable all metrics
  (Note: make sure to quote the asterisk `'*'`, otherwise it might be expanded by the shell)
- `SCOREP_METRIC_TOPDOWN_PLUGIN_INTERVAL_US=500` (optional, default 500): minimum time between two samples in microseconds (sampling below this threshold will be refused)

The full configuration used for test setups can be found below.

## Building
Use the usual CMake build process:

```bash
git clone https://github.com/score-p/scorep_plugin_topdown
cd scorep_plugin_topdown
# initialize submodules (!!)
git submodule update --init --recursive

# vanilla cmake build process:
mkdir build && cd build
cmake ..
make -j $((2*$(nproc)))
```

The resulting `libtopdown_plugin.so` must be placed in the `LD_LIBRARY_PATH` to be found by Score-P.

## Example Setup
```bash
# general scorep settings
export SCOREP_ENABLE_PROFILING=0
export SCOREP_ENABLE_TRACING=1
export SCOREP_TOTAL_MEMORY=4095M

# configure topdown plugin
export SCOREP_METRIC_PLUGINS=topdown_plugin
export SCOREP_METRIC_TOPDOWN_PLUGIN='*'

# pin to P-cores (alderlake-specific)
export OMP_NUM_THREADS=16
export GOMP_CPU_AFFINITY=0-15
```

## License
This plugin is available under the [BSD-3-clause license](./LICENSE).
