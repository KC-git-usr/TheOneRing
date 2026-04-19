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

## Compiling and running the application

Option #1: good old cmake
```
* cd <root_cmakelists>
* mkdir build && cd build
* cmake -S ../.
* cmake --build .
* sudo su
* ./the_one_ring
```
Option #2: using colcon
```
* cd <your_workspace>
* colcon build --packages-select the_one_ring
* cd install/the_one_ring
```
