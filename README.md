What's This?
------
This is a simple demonstration of using the Intel built-in perf
counters. The basic idea is to create one or more `PerfCounter`
objects before doing something "interesting", and read the `value()`
afterward, perhaps logging unusally high events or aggregating
readings over time.

Getting it to work
------
For a Linux server, make sure the following configuration allows reading
performance counters from user mode:

   - `/proc/sys/kernel/perf_event_paranoid`: set to -1 or 0
   - `/sys/devices/cpu/rdpmc`: set to 2, newer kernels only

To make Google benchmark happy, you'll want to disable cpu scaling.
    cpupower frequency-set -g performance

Run the sample with:

    ./benchy --benchmark_counters_tabular=true

Trouble?
----

    munmap(_meta, pgsz) returned Invalid argument

Double check `/proc/sys/kernel/perf_event_paranoid` is set to -1
