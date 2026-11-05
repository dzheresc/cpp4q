// Chapter 20 demo: the same price from 1, 2, 4 and 8 threads, and the
// speed-up.

#include <chrono>
#include <cstdio>
#include <initializer_list>
#include <thread>

#include "ch20_threads/parallel_pricer.h"

namespace a = cpp4q::armstrong;

int main() {
  const a::ch20::BlackScholesModel model;
  const a::ch12::ArithmeticAsianOption asian(a::ch12::OptionType::kCall, 100,
                                             1);
  a::ch20::ParallelSettings s{.n_scenarios = 200'000, .n_steps = 12};
  std::printf("arithmetic Asian call, %d paths, %d fixings, hardware "
              "threads %u\n",
              s.n_scenarios, s.n_steps, std::thread::hardware_concurrency());
  std::printf("%8s %20s %12s %10s\n", "threads", "price", "std err", "ms");
  for (int t : {1, 2, 4, 8}) {
    s.n_threads = t;
    const auto start = std::chrono::steady_clock::now();
    const auto e = a::ch20::PriceParallel(asian, model, s);
    const auto ms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start)
                        .count();
    std::printf("%8d %20.15f %12.6f %10.1f\n", t, e.price, e.std_error, ms);
  }
  s.n_threads = 4;
  std::printf("std::async:  %.15f\nmutex:       %.15f\n",
              a::ch20::PriceWithAsync(asian, model, s).price,
              a::ch20::PriceWithMutex(asian, model, s).price);
  return 0;
}
