# Project: The One Ring
Handcrafted algorithm for 4000Hz cycle frequency (250 us) timer, on tuned PREEMPT_RT Linux Ubuntu 24.04, using C++20 features, meant for real-time robotic control applications.

This implementation sets up a timer that broadcasts a tick at a base rate of 4000Hz, and any number of tasks (real-time or non-real time tasks) listening to this broadcast can rely on this timer to do periodic work. For example: your cyclic control algorithm running on thread #1, your EtherCAT bus cycle time running on thread #2 and your DDS pub/sub running on thread #3 can all rely on the same timer tick.

'One Timer to rule all cyclic tasks, One Timer to find them,
One Timer to bring them all and in the darkness bind them.'

## Tuning the Linux Kernel
0. Tune BOIS:
In general you wanna turn off power savings (ACPI). The BIOS settings is vendor specific, but these are the options I turned off on my PC:
```
* Advanced > ACPI Settings > Enable Hibernation > Disable
* IntelRCSetup > Processor Config > Hyper-Threading[ALL] > Disable
* IntelRCSetup > Processor Config > VMX > Disable
* IntelRCSetup > Adv. Power Management Config > Power Tech > Disable
```
1. Get the Linux Kernel with the PREEMPT_RT patch. If choice of OS is Ubuntu 24.04, you can use the pro subscription to enable the PREEMPT_RT patch.
2. Tune the below parameters via your `/etc/default/grub`, run $`sudo apt update-grub` and restart.
```
# KR comment: Real Time apps are meant to be run on CPU core number 2 and 3 (0 based indexing). Seperate housekeeping cores from RT cores
# KR commnet: IRQ affinity is set to CPU core number 0
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash cpuidle.off=1 intel_idle.max_cstate=0 cpufreq.off=1 processor.max_cstate=0 intel_pstate=disable idle=poll nohz_full=2,3 isolcpus=2,3 rcu_nocbs=2,3 irqaffinity=0" #Changed by KR
GRUB_CMDLINE_LINUX="text" #Changed by KR
```
3. [Optional] If you wanna benchmark the maximum thread wake up latency due to the effect of a kernel parameter that was changed
```
# git clone cyclictest, run this command, and see performance via the histogram
cyclictest -Sp99 -i1000 -h1000 -m -q -D24h > histogram_24hr.log
```
```
# synthetic load to be run WHILE cyclictest is running
stress-ng --matrix 0 -t 1h
# stress-ng heats up the CPU a lot, so run it for 1hr, let it cool for say 30min, and run it again manually
```

## Compiling and Running the Application

### Option #1: Good Old CMake

> **This project requires Clang.** If Clang is not your default compiler, prefix every
> `cmake` configure command with `CC=clang CXX=clang++`.
> Always use separate build directories for sanitizer/coverage variants — they are incompatible with each other.

**Basic Debug Build:**
```bash
cd the_one_ring

# Modern out-of-source build (cmake creates the build/ directory automatically)
CC=clang CXX=clang++ cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
sudo ./build/the_one_ring

# Or equivalently, the traditional way:
# mkdir build && cd build
# CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug ..
# cmake --build . -j$(nproc)
# sudo ./the_one_ring
```

**With Sanitizers (ASan + UBSan):**
```bash
CC=clang CXX=clang++ cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_ASAN=ON \
  -DENABLE_UBSAN=ON
cmake --build build-asan -j$(nproc)
```

**With ThreadSanitizer** (separate build — can't mix with ASan):
```bash
CC=clang CXX=clang++ cmake -S . -B build-tsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_TSAN=ON
cmake --build build-tsan -j$(nproc)
```

**With RealtimeSanitizer:**
```bash
CC=clang CXX=clang++ cmake -S . -B build-rtsan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_RTSAN=ON
cmake --build build-rtsan -j$(nproc)
```

**With Tests:**
```bash
CC=clang CXX=clang++ cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

**With Code Coverage:**
```bash
CC=clang CXX=clang++ cmake -S . -B build-cov \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  -DENABLE_COVERAGE=ON
cmake --build build-cov -j$(nproc)
cd build-cov && LLVM_PROFILE_FILE=$(pwd)/default.profraw ctest --output-on-failure
cmake --build . --target coverage-html    # browse coverage/html/index.html
cmake --build . --target coverage-summary # prints per-file % to stdout
```

**With Static Analysis:**
```bash
CC=clang CXX=clang++ cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_CLANG_TIDY=ON \
  -DENABLE_CPPCHECK=ON
cmake --build build -j$(nproc)
```

**Release Build:**
```bash
CC=clang CXX=clang++ cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j$(nproc)
```


### Option #2: Using Colcon

Colcon discovers the package via `package.xml`. Your workspace should look like:
```
code_ws/
  src/
    TheOneRing/
      the_one_ring/
        CMakeLists.txt
        package.xml
        src/
        ...
```

**Basic Build:**
```bash
cd ~/code_ws
CC=clang CXX=clang++ colcon build --packages-select the_one_ring
```

**With CMake Args** (sanitizers, tests, etc.):
```bash
CC=clang CXX=clang++ colcon build --packages-select the_one_ring \
  --cmake-args \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON \
    -DBUILD_TESTING=ON
```

**Run the binary:**
```bash
source install/setup.bash
sudo ./install/the_one_ring/lib/the_one_ring/the_one_ring
```

**Run tests (if built with `-DBUILD_TESTING=ON`):**
```bash
colcon test --packages-select the_one_ring
colcon test-result --verbose
```
