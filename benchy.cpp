#include "benchmark/benchmark.h"

#include <cstring>
#include <thread>
#include <vector>
#include <functional>

#include "perf_counter.h"

static void BM_memcpy(benchmark::State& state) {
    char* src = new char[state.range(0)];
    char* dst = new char[state.range(0)];
    memset(src, 'x', state.range(0));
    while (state.KeepRunning())
        memcpy(dst, src, state.range(0));
    state.SetComplexityN(state.range(0));
    state.SetBytesProcessed(int64_t(state.iterations()) *
            int64_t(state.range(0)));
    delete[] src;
    delete[] dst;
}

BENCHMARK(BM_memcpy)->Range(8, 8<<14)->Complexity();

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
    int sz = state.range(1);
    int nThreads = state.range(0);

    int* src1 = new int[sz*sz];
    int* src2 = new int[sz*sz];
    int* dst = new int[sz*sz];

    PerfCounter perf1(HW_CPU_CYCLES);
    PerfCounter perf2(HW_INSTRUCTIONS);
    PerfCounter perf3(HW_CACHE_REFERENCES);
    PerfCounter perf4(HW_CACHE_MISSES);
    PerfCounter perf5(SW_CONTEXT_SWITCHES);
    PerfCounter perf6(SW_CPU_MIGRATIONS);

    uint32_t count = 0u;

    while (state.KeepRunning()) {
        mat_mult(sz, nThreads, src1, src2, dst);
        ++count;
        benchmark::ClobberMemory();
    }

    state.counters["cycles"] = perf1.value() / count;
    state.counters["instructions"] = perf2.value() / count;
    state.counters["cahce_refs"] = perf3.value() / count;
    state.counters["cahce_misses"] = perf4.value() / count;
    state.counters["ctxt_switch"] = perf5.value() / count;
    state.counters["cpu_migs"] = perf6.value() / count;
    state.SetComplexityN(sz);

    delete[] src1;
    delete[] src2;
    delete[] dst;
}

BENCHMARK(BM_matrix)->RangeMultiplier(2)->Ranges({{1,8}, {8, 1<<10}})->Complexity();

static void BM_empty(benchmark::State& state) {
    PerfCounter perf1(HW_CPU_CYCLES);
    PerfCounter perf2(HW_INSTRUCTIONS);
    PerfCounter perf3(HW_CACHE_REFERENCES);
    PerfCounter perf4(HW_CACHE_MISSES);
    PerfCounter perf5(SW_CONTEXT_SWITCHES);
    PerfCounter perf6(SW_CPU_MIGRATIONS);

    uint32_t count = 0u;

    while (state.KeepRunning()) {
        ++count;
        benchmark::ClobberMemory();
    }

    state.counters["cycles"] = perf1.value() / count;
    state.counters["instructions"] = perf2.value() / count;
    state.counters["cahce_refs"] = perf3.value() / count;
    state.counters["cahec_misses"] = perf4.value() / count;
    state.counters["ctxt_switch"] = perf5.value() / count;
    state.counters["cpu_migs"] = perf6.value() / count;
}

BENCHMARK(BM_empty);

BENCHMARK_MAIN()
