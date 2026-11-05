// Chapter 10 demo: one function, three execution policies.

#include <chrono>
#include <cstdio>
#include <execution>
#include <vector>

#include "ch10_parallel_algorithms/portfolio_algos.h"

namespace {

template <typename F>
double TimeMs(F&& f) {
  const auto t0 = std::chrono::steady_clock::now();
  f();
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - t0)
      .count();
}

}  // namespace

int main() {
  namespace ch10 = cpp4q::williams::ch10;
  const auto book = ch10::MakeBondBook(2'000'000, 1);
  std::printf("parallel backend: %s\n",
              ch10::ParallelBackendAvailable() ? "oneTBB" : "none (serial)");

  double pv_seq = 0, pv_par = 0, pv_unseq = 0;
  const double t_seq = TimeMs(
      [&] { pv_seq = ch10::PortfolioPv(std::execution::seq, book, 0.04); });
  const double t_par = TimeMs(
      [&] { pv_par = ch10::PortfolioPv(std::execution::par, book, 0.04); });
  const double t_unseq = TimeMs([&] {
    pv_unseq = ch10::PortfolioPv(std::execution::par_unseq, book, 0.04);
  });
  std::printf(
      "PV seq %.6e (%.1f ms) | par %.6e (%.1f ms) | par_unseq %.6e "
      "(%.1f ms)\n",
      pv_seq, t_seq, pv_par, t_par, pv_unseq, t_unseq);

  auto sorted = book;
  ch10::SortByNotional(std::execution::par, sorted);
  std::printf("largest notional %.0f (id %lld)\n", sorted.front().notional,
              static_cast<long long>(sorted.front().id));
}
