#pragma once

#include <cstdint>
#include <vector>
#include <string>

enum CpuCounter {
    HW_CPU_CYCLES,
    HW_INSTRUCTIONS,
    HW_CACHE_REFERENCES,
    HW_CACHE_MISSES,
    HW_BRANCH_MISSES,
    HW_BRANCH_INSTRUCTIONS,
    HW_BUS_CYCLES,
    HW_STALLED_CYCLES_FRONTEND,
    HW_STALLED_CYCLES_BACKEND,

    SW_CONTEXT_SWITCHES,
    SW_CPU_MIGRATIONS
};

struct perf_event_mmap_page;

class PerfCounter {
    bool _enabled;
    uint64_t _start;
    int _fd;
    perf_event_mmap_page * _meta;
    bool _hardware;

public:
    PerfCounter(CpuCounter counter = HW_INSTRUCTIONS);
    ~PerfCounter();

    uint64_t value();

    void reset();
    void enable();
    void disable();

    static bool check_config(std::vector<std::string> &messages);
};
