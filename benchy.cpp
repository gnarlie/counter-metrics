#include "benchmark/benchmark.h"

#include <cstring>
#include <thread>
#include <vector>
#include <functional>

#include "perf_counter.h"

static void BM_memcpy(benchmark::State& state) {
    PerfCounter perf1(CpuCounter(0x4124));
    PerfCounter perf2(CpuCounter(0xd824));
    PerfCounter perf3(HW_CACHE_REFERENCES);
    PerfCounter perf4(HW_CACHE_MISSES);

    auto src = std::make_unique<char[]>(state.range(0));
    auto dst = std::make_unique<char[]>(state.range(0));
    memset(src.get(), 'x', state.range(0));

    auto count = 0u;
    while (state.KeepRunning()) {
        memcpy(dst.get(), src.get(), state.range(0));
        ++count;
        benchmark::ClobberMemory();
    }

    state.counters["l2_demand_data_rd"] = perf1.value() / count;
    state.counters["l2_prefetch"] = perf2.value() / count;
    state.counters["cache_refs"] = perf3.value() / count;
    state.counters["cache_misses"] = perf4.value() / count;
}

BENCHMARK(BM_memcpy)->Range(8, 8<<24);

void mat_mult(size_t sz, size_t nThreads, const int * a, const int * b, int * dest) {
    std::vector<std::thread> threads(nThreads-1);
    auto chunk = sz / nThreads;

    auto work = [=](int t) {

                    for (auto k = chunk*t; k < std::min(chunk*(t+1), sz); ++k) {
                        for (auto i = 0; i < sz; ++i) {
                            int accum = 0;
                            for (auto j = 0; j < sz; ++j) {
                                accum += a[k * sz + j] + b[j*sz + i];
                            }
                            dest[k * sz + i] = accum;
                        }
                    }
                };

    for (auto t = 1; t < nThreads; ++t) {
        std::thread next(std::bind(work, t));
        threads[t-1] = std::move(next);
    }

    work(0);

    for(auto &t : threads) {
        t.join();
    }

}

static void BM_matrix(benchmark::State& state) {
    std::vector<std::string> errs;
    if (!PerfCounter::check_config(errs)) {
        state.SkipWithError(errs[0].c_str());
    }

    auto sz = state.range(1);
    auto nThreads = state.range(0);

    auto src1 = std::make_unique<int[]>(sz*sz);
    auto src2 = std::make_unique<int[]>(sz*sz);
    auto dst = std::make_unique<int[]>(sz*sz);

    PerfCounter perf1(HW_CPU_CYCLES);
    PerfCounter perf2(HW_INSTRUCTIONS);
    PerfCounter perf3(HW_CACHE_REFERENCES);
    PerfCounter perf4(HW_CACHE_MISSES);
    PerfCounter perf5(SW_CONTEXT_SWITCHES);
    PerfCounter perf6(SW_CPU_MIGRATIONS);

    auto count = 0u;

    while (state.KeepRunning()) {
        mat_mult(sz, nThreads, src1.get(), src2.get(), dst.get());
        ++count;
        benchmark::ClobberMemory();
    }

    if (count) {
        state.counters["cycles"] = perf1.value() / count;
        state.counters["instructions"] = perf2.value() / count;
        state.counters["cahce_refs"] = perf3.value() / count;
        state.counters["cahce_misses"] = perf4.value() / count;
        state.counters["ctxt_switch"] = perf5.value() / count;
        state.counters["cpu_migs"] = perf6.value() / count;
        state.SetComplexityN(sz);
    }
}

BENCHMARK(BM_matrix)->RangeMultiplier(2)->Ranges({{1,8}, {8, 1<<10}})->Complexity();

static void BM_empty(benchmark::State& state) {
    PerfCounter perf1(HW_CPU_CYCLES);
    PerfCounter perf2(HW_INSTRUCTIONS);
    PerfCounter perf3(HW_CACHE_REFERENCES);
    PerfCounter perf4(HW_CACHE_MISSES);
    PerfCounter perf5(SW_CONTEXT_SWITCHES);
    PerfCounter perf6(SW_CPU_MIGRATIONS);

    auto count = 0u;

    while (state.KeepRunning()) {
        ++count;
        benchmark::ClobberMemory();
    }

    state.counters["cycles"] = perf1.value() / count;
    state.counters["instructions"] = perf2.value() / count;
    state.counters["cahce_refs"] = perf3.value() / count;
    state.counters["cahce_misses"] = perf4.value() / count;
    state.counters["ctxt_switch"] = perf5.value() / count;
    state.counters["cpu_migs"] = perf6.value() / count;
}

BENCHMARK(BM_empty);

BENCHMARK_MAIN()
