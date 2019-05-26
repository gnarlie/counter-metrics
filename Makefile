benchy  : benchy.cpp perf_counter.cpp | perf_counter.h
	$(CXX) -DNDEBUG -std=c++17 -g -O3 -o $@ $^ benchmark/build/src/libbenchmark.a -I benchmark/include -l pthread
