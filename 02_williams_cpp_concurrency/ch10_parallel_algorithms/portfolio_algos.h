// Chapter 10: Parallel algorithms from the standard library.
//
// Every function takes an execution policy so the same code runs as
// std::execution::seq, par or par_unseq; the benchmarks compare them.
// With libstdc++ the parallel policies need oneTBB at link time; without it
// they quietly run serially.
//
// Rules the chapter stresses, which this code follows:
// * element access functions must not race: each call writes only to its
//   own element or returns a value for the reduction;
// * under par_unseq they must not lock or allocate either, because calls
//   may be interleaved on one thread (vectorised).
//
// The policy is passed on as an lvalue on purpose: libstdc++ 13's PSTL
// glue fails to compile when handed an rvalue policy such as `par{}`.

#ifndef CPP4Q_WILLIAMS_CH10_PARALLEL_ALGORITHMS_PORTFOLIO_ALGOS_H_
#define CPP4Q_WILLIAMS_CH10_PARALLEL_ALGORITHMS_PORTFOLIO_ALGOS_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <execution>
#include <functional>
#include <numeric>
#include <span>
#include <type_traits>
#include <vector>

namespace cpp4q::williams::ch10 {

template <typename P>
concept ExecutionPolicy = std::is_execution_policy_v<std::remove_cvref_t<P>>;

struct Bond {
  std::int64_t id = 0;
  double notional = 0.0;
  double coupon = 0.0;  // annual rate, paid annually
  int years = 0;        // years to maturity
};

// Flat-yield PV of an annual-coupon bullet bond.
[[nodiscard]] inline double BondPv(const Bond& b, double yield) noexcept {
  const double df1 = 1.0 / (1.0 + yield);
  double df = 1.0, pv = 0.0;
  for (int t = 1; t <= b.years; ++t) {
    df *= df1;
    pv += b.coupon * b.notional * df;
  }
  return pv + b.notional * df;
}

template <ExecutionPolicy P>
[[nodiscard]] double PortfolioPv(P&& policy, std::span<const Bond> bonds,
                                 double yield) {
  return std::transform_reduce(
      policy, bonds.begin(), bonds.end(), 0.0, std::plus<>{},
      [yield](const Bond& b) { return BondPv(b, yield); });
}

// Fills `out[i]` with the PV of bonds[i]; out must have bonds.size() slots.
template <ExecutionPolicy P>
void PriceAll(P&& policy, std::span<const Bond> bonds, std::span<double> out,
              double yield) {
  std::transform(policy, bonds.begin(), bonds.end(), out.begin(),
                 [yield](const Bond& b) { return BondPv(b, yield); });
}

// Largest notional first; ties by id so the result is deterministic.
template <ExecutionPolicy P>
void SortByNotional(P&& policy, std::vector<Bond>& bonds) {
  std::sort(
      policy, bonds.begin(), bonds.end(), [](const Bond& a, const Bond& b) {
        return a.notional != b.notional ? a.notional > b.notional : a.id < b.id;
      });
}

template <ExecutionPolicy P>
[[nodiscard]] std::size_t CountLimitBreaches(P&& policy,
                                             std::span<const double> exposures,
                                             double limit) {
  return static_cast<std::size_t>(
      std::count_if(policy, exposures.begin(), exposures.end(),
                    [limit](double e) { return std::abs(e) > limit; }));
}

// True if any trade's exposure is NaN or infinite (std::any_of stops
// early, in parallel too).
template <ExecutionPolicy P>
[[nodiscard]] bool HasBadExposure(P&& policy,
                                  std::span<const double> exposures) {
  return std::any_of(policy, exposures.begin(), exposures.end(),
                     [](double e) { return !std::isfinite(e); });
}

// Deterministic synthetic book for tests, demos and benchmarks.
[[nodiscard]] std::vector<Bond> MakeBondBook(std::size_t n, std::uint64_t seed);

// Whether this build's parallel policies actually run in parallel.
[[nodiscard]] bool ParallelBackendAvailable();

}  // namespace cpp4q::williams::ch10

#endif  // CPP4Q_WILLIAMS_CH10_PARALLEL_ALGORITHMS_PORTFOLIO_ALGOS_H_
