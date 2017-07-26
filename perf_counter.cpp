#include "perf_counter.h"

#include <fstream>
#include <iostream>
#include <tuple>

#include <linux/perf_event.h>
#include <sys/mman.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>

#include <strings.h>
#include <string.h>

namespace {
    bool check_okay() {
        std::ifstream proc_event_paranoid("/proc/sys/kernel/perf_event_paranoid");
        int n = 2;
        proc_event_paranoid >> n;

        if (proc_event_paranoid.bad() || n > 0) {
            std::cerr << "Try tweeking /proc/sys/kernel/perf_event_paranoid - it should be 0 or -1" << std::endl;
            return false;
        }

        std::ifstream cpu_rdpmc("/sys/devices/cpu/rdpmc");
        if (cpu_rdpmc.good()) {
            cpu_rdpmc >> n;

            if (cpu_rdpmc.bad() || n != 2) {
                std::cerr << "Try tweeking /sys/devices/cpu/rdpmc - it should be 2" << std::endl;
                return false;
            }
        }

        return true;
    }

    int64_t rdpmc(int64_t c) {
        uint32_t edx, eax;
        asm volatile("rdpmc \n\t"
                     : "=d" (edx), "=a" (eax): "c" (c) );

        return ((uint64_t)edx << 32) | eax;
    }

    void barrier() {
        asm volatile("" :::"memory");
    }

    uint64_t perf_event_open(struct perf_event_attr *event,
                             pid_t pid, int cpu,
                             int group = -1, uint64_t flags = 0) {
        return syscall(__NR_perf_event_open, event, pid, cpu, group, flags);
    }

    std::pair<long, long> translate(CpuCounter counter) {
#define HW(c) case(c): return std::make_pair(PERF_TYPE_HARDWARE, PERF_COUNT_ ## c)
#define SW(c) case(c): return std::make_pair(PERF_TYPE_SOFTWARE, PERF_COUNT_ ## c)

        switch(counter) {
            HW(HW_CPU_CYCLES);
            HW(HW_INSTRUCTIONS);
            HW(HW_CACHE_REFERENCES);
            HW(HW_CACHE_MISSES);
            HW(HW_BRANCH_MISSES);
            HW(HW_BRANCH_INSTRUCTIONS);
            HW(HW_BUS_CYCLES);
            HW(HW_STALLED_CYCLES_FRONTEND);
            HW(HW_STALLED_CYCLES_BACKEND);

            SW(SW_CONTEXT_SWITCHES);
            SW(SW_CPU_MIGRATIONS);

            default:
                return std::make_pair(PERF_TYPE_RAW, counter);
        }
#undef HW
#undef SW
    }
}

template<typename T>
T do_c_check(T rc, const char * expr, int line) {
    if ((long)rc == -1) {
        std::cerr << "On line " << line << ", " << expr  << " returned " << strerror(errno) << std::endl;
    }

    return rc;
};

#define c_check(expr) do_c_check((expr), #expr, __LINE__)

PerfCounter::PerfCounter(CpuCounter counter) : _fd(0) {
    static bool okay = check_okay();


    _enabled = okay;
    _hardware = counter <= HW_STALLED_CYCLES_BACKEND;
    if (okay) {
        perf_event_attr pea;
        bzero(&pea, sizeof(pea));

        std::tie(pea.type, pea.config) = translate(counter);
        pea.size = sizeof(pea);
        pea.disabled = 1;
        pea.exclude_kernel = 0;
        pea.exclude_hv = 0;

        _fd = c_check(perf_event_open(&pea, 0, -1));
        _enabled = okay && _fd != -1;

        auto pgsz = sysconf(_SC_PAGESIZE);
        _meta = reinterpret_cast<perf_event_mmap_page*>(c_check(mmap(nullptr, pgsz, PROT_READ, MAP_SHARED, _fd, 0)));

        if (_enabled) {
            c_check(ioctl(_fd, PERF_EVENT_IOC_ENABLE, 0));

            reset();
        }
    }
}

void PerfCounter::reset() {
    if (_enabled) {
        ioctl(_fd, PERF_EVENT_IOC_RESET);
        _start = 0;
        _start = value();
    }
}

uint64_t PerfCounter::value() {
    if (_enabled && _meta) {
        if (_hardware) {
            uint64_t seq, now;
            do {
                seq = _meta->lock;
                barrier();
                now = rdpmc(_meta->index - 1);
                barrier();
            }  while (_meta->lock != seq) ;

            return now - _start;
        }
        else {
            uint64_t now;
            c_check(read(_fd, &now, sizeof(now)));
            return now - _start;
        }
    }
    else {
        return 0;
    }
}

void PerfCounter::enable() {
    c_check(ioctl(_fd, PERF_EVENT_IOC_ENABLE, 0));
}

void PerfCounter::disable() {
    c_check(ioctl(_fd, PERF_EVENT_IOC_DISABLE, 0));
}

PerfCounter::~PerfCounter() {
    if (_enabled) {

        ioctl(_fd, PERF_EVENT_IOC_DISABLE);
        close(_fd);
    }
}
