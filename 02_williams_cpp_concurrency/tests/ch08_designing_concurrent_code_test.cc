#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "ch08_designing_concurrent_code/parallel_algorithms.h"

namespace cpp4q::williams::ch08 {
namespace {

TEST(ParallelForEachTest, VisitsEveryElementOnce) {
  std::vector<int> v(100'000, 1);
  ParallelForEach(v.begin(), v.end(), [](int& x) { x *= 3; }, 4, 100);
  EXPECT_TRUE(std::all_of(v.begin(), v.end(), [](int x) { return x == 3; }));
}

TEST(ParallelForEachTest, PropagatesException) {
  std::vector<int> v(10'000);
  std::iota(v.begin(), v.end(), 0);
  EXPECT_THROW(ParallelForEach(
                   v.begin(), v.end(),
                   [](int x) {
                     if (x == 1234) throw std::runtime_error("bad trade");
                   },
                   4, 100),
               std::runtime_error);
}

TEST(ParallelFindTest, FindsFirstMatchLikeStdFindIf) {
  std::vector<int> v(1'000'000, 0);
  v[700'000] = 1;
  v[300'001] = 1;
  v[999'999] = 1;
  for (unsigned t : {1u, 2u, 4u, 7u}) {
    const auto it =
        ParallelFind(v.begin(), v.end(), [](int x) { return x == 1; }, t, 1000);
    EXPECT_EQ(it - v.begin(), 300'001) << t;
  }
  EXPECT_EQ(ParallelFind(v.begin(), v.end(), [](int x) { return x == 2; }),
            v.end());
  std::vector<int> empty;
  EXPECT_EQ(ParallelFind(empty.begin(), empty.end(), [](int) { return true; }),
            empty.end());
}

TEST(ParallelScanTest, MatchesInclusiveScan) {
  for (std::size_t n : {1u, 5u, 100'000u, 1'000'003u}) {
    std::vector<long long> in(n);
    std::iota(in.begin(), in.end(), 1);
    std::vector<long long> expected(n), out(n);
    std::inclusive_scan(in.begin(), in.end(), expected.begin());
    for (unsigned t : {1u, 3u, 4u}) {
      std::fill(out.begin(), out.end(), 0);
      ParallelInclusiveScan<long long>(in, out, t, 1000);
      EXPECT_EQ(out, expected) << n << " " << t;
    }
  }
}

TEST(FalseSharingTest, PackedAndPaddedAgree) {
  std::vector<std::int32_t> ids(100'000);
  std::iota(ids.begin(), ids.end(), 0);
  const auto a = CountByBucketPacked(ids, 4);
  const auto b = CountByBucketPadded(ids, 4);
  EXPECT_EQ(a, b);
  EXPECT_EQ(a, (std::vector<std::int64_t>{25'000, 25'000, 25'000, 25'000}));
}

TEST(ScenarioVarTest, MatchesSerialComputation) {
  constexpr std::size_t kFactors = 4, kScen = 1000;
  const std::vector<double> exposures{1e6, -5e5, 2e5, 3e5};
  std::vector<double> scen(kFactors * kScen);
  std::mt19937_64 rng(1);
  std::normal_distribution<double> n(0.0, 0.01);
  for (auto& s : scen) s = n(rng);

  std::vector<double> pnl(kScen);
  for (std::size_t s = 0; s < kScen; ++s) {
    for (std::size_t k = 0; k < kFactors; ++k) {
      pnl[s] += exposures[k] * scen[s * kFactors + k];
    }
  }
  std::sort(pnl.begin(), pnl.end());
  EXPECT_NEAR(ScenarioVar(exposures, scen, 0.99, 4), -pnl[10], 1e-6);
  EXPECT_THROW(ScenarioVar(exposures, {}, 0.99), std::invalid_argument);
  EXPECT_THROW(ScenarioVar(exposures, scen, 1.0), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::williams::ch08
