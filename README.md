# Clone the repo


```bash
git clone https://github.com/SamMaoYS/gem5-baseline --recursive
```

## Build

```bash
scons -j [Number of Threads] build/X86/gem5.opt CPU_MODELS='AtomicSimpleCPU,O3CPU,TimingSimpleCPU,MinorCPU'
```

## DBCP
The implementation of DBCP is located in `src/mem/cache/prefetch/dbcp.hh`, `src/mem/cache/prefetch/dbcp.cc`, `src/mem/cache/prefetch/Prefetcher.py`. `src/mem/cache/prefetch/SConscript`

In order to invoke the DBCP to the LLC, in the config file `run_spec/gem5-config/system.py`. in L1 data cache set the prefetcher as follows:

```python
tags = BaseSetAssoc()
prefetcher = DBCPPrefetcher()
```

## HawkEye
The implementation of the HawkEye is located in `src/mem/cache/replacement_policies/hawkeye_rp.hh`, `src/mem/cache/replacement_policies/hawkeye_rp.cc`, `src/mem/cache/replacement_policies/ReplacementPolicies.py`. `src/mem/cache/replacement_policies/SConscript`

In order to invoke the HawkEye to the LLC, there is a configuration file located under `run_spec/gem5-config-*/system.py`. There are many directories corresponding to different configurations. "-baseline-dbcp" indicates the baseline (LRU) configuration for DBCP code, "-baseline-he" indicates the baseline (LRU) configuration for HawkEye code. "-stride" and "-tagged" indicates the configuration using stride and tagged prefetch policy respectively. "-dbcp" and "-he" indicates the actual configuration that utilizes DBCP and HawkEye. In this file, each cache is declared as a class inherits the original class of Cache. For a particular cache, the replacement policy can be set using the following line

```python
replacement_policy = HawkEyeRP()
```

And the prefetching policy can be set using the following line:
```python
tags = BaseSetAssoc()
prefetcher = DBCPPrefetcher()
```

Usually, you can run the code directly by setting the environment variable in the run_spec directory of this repo

```bash
export M5_PATH=../
export LAB_PATH=$PWD
```

and then execute the python file

```bash
python3 launch-project-*-boi.py [Number of threads]
```

The python files are configured to look for SPEC 2017 file in `/data/benchmarks/spec2017/` which is on the arch1 research machine of SFU Architecture Lab. Please change that if you're running on a different machine

The result will be outputed to `results-*/` directory. You can use `extract*.py` to extract them into CSV files.

The SPEC 2017 benchmark is configured to be executed on the gem5 simulator in order to evaluate the replacement policy.


## About gem5 simulator Baseline
This is the gem5 simulator.

The main website can be found at http://www.gem5.org

A good starting point is http://www.gem5.org/about, and for
more information about building the simulator and getting started
please see http://www.gem5.org/documentation and
http://www.gem5.org/documentation/learning_gem5/introduction.

To build gem5, you will need the following software: g++ or clang,
Python (gem5 links in the Python interpreter), SCons, SWIG, zlib, m4,
and lastly protobuf if you want trace capture and playback
support. Please see http://www.gem5.org/documentation/general_docs/building
for more details concerning the minimum versions of the aforementioned tools.

Once you have all dependencies resolved, type 'scons
build/<ARCH>/gem5.opt' where ARCH is one of ARM, NULL, MIPS, POWER, SPARC,
or X86. This will build an optimized version of the gem5 binary (gem5.opt)
for the the specified architecture. See
http://www.gem5.org/documentation/general_docs/building for more details and
options.

The basic source release includes these subdirectories:
   - configs: example simulation configuration scripts
   - ext: less-common external packages needed to build gem5
   - src: source code of the gem5 simulator
   - system: source for some optional system software for simulated systems
   - tests: regression tests
   - util: useful utility programs and files

To run full-system simulations, you will need compiled system firmware
(console and PALcode for Alpha), kernel binaries and one or more disk
images.

If you have questions, please send mail to gem5-users@gem5.org

Enjoy using gem5 and please share your modifications and extensions.
