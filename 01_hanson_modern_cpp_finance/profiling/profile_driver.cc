// Fixed, deterministic workloads for valgrind (callgrind / cachegrind /
// massif). Usage: hanson_profile <workload>; run without arguments to list.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string_view>
#include <vector>

#include "ch03_polymorphism/payoff.h"
#include "ch04_stl_containers/order_book.h"
#include "ch05_algorithms_ranges/return_stats.h"
#include "ch06_random_concurrency/monte_carlo.h"
#include "ch07_dates_fixed_income/bond.h"
#include "ch08_linear_algebra/decompositions.h"
#include "ch08_linear_algebra/matrix.h"
#include "ch10_modules_concepts/generic_pricing.h"

namespace {

namespace hanson = cpp4q::hanson;

double g_sink = 0.0;  // keeps results observable

std::vector<double> Prices(std::size_t n) {
  std::mt19937_64 rng(3);
  std::normal_distribution<double> z(0.0, 0.01);
  std::vector<double> p(n);
  double s = 100.0;
  for (auto& x : p) x = (s *= std::exp(z(rng)));
  return p;
}

void Stats() {
  const auto p = Prices(200'000);
  const auto r = hanson::ch05::SimpleReturns(p);
  g_sink += hanson::ch05::SampleStdDev(r) + hanson::ch05::MaxDrawdown(p) +
            hanson::ch05::HistoricalVar(r, 0.99);
  g_sink += hanson::ch05::MovingAverage(p, 200).back();
}

void MonteCarlo() {
  const hanson::ch06::GbmParams gbm{100.0, 0.03, 0.0, 0.2, 1.0};
  g_sink += hanson::ch06::PriceAsianMc(hanson::ch06::OptionType::kCall, 100.0,
                                       gbm, 52, {.num_paths = 20'000})
                .price;
  g_sink += hanson::ch06::PriceEuropeanMc(hanson::ch06::OptionType::kPut, 100.0,
                                          gbm, {.num_paths = 200'000})
                .price;
}

void MonteCarloParallel() {
  const hanson::ch06::GbmParams gbm{100.0, 0.03, 0.0, 0.2, 1.0};
  g_sink +=
      hanson::ch06::PriceAsianMcParallel(hanson::ch06::OptionType::kCall, 100.0,
                                         gbm, 52, {.num_paths = 20'000}, 4)
          .price;
}

void Matrix() {
  std::mt19937_64 rng(1);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  hanson::ch08::Matrix a(256, 256);
  hanson::ch08::Matrix b(256, 256);
  for (std::size_t i = 0; i < 256; ++i) {
    for (std::size_t j = 0; j < 256; ++j) {
      a(i, j) = u(rng);
      b(i, j) = u(rng);
    }
  }
  const auto c = a * b;
  const auto spd = a * a.Transpose() + hanson::ch08::Matrix::Identity(256);
  const auto l = hanson::ch08::Cholesky(spd);
  g_sink += c(7, 7) + l(255, 255);
}

void Tree() {
  const hanson::ch10::TreeParams<double> p{100.0, 0.05, 0.0, 0.2, 1.0, 1000};
  g_sink += hanson::ch10::PriceBinomial(p, hanson::ch03::Put{100.0},
                                        hanson::ch10::ExerciseStyle::kAmerican);
}

void Bonds() {
  using namespace std::chrono;  // NOLINT(build/namespaces)
  namespace ch07 = hanson::ch07;
  const ch07::Calendar cal;
  for (int i = 0; i < 200; ++i) {
    const ch07::FixedRateBond bond(
        100.0, 0.03 + 0.0001 * i,
        {.effective = 2020y / January / 15d, .maturity = 2050y / January / 15d},
        ch07::DayCount::kActualActualIsda, cal);
    g_sink += bond.YieldFromCleanPrice(2025y / March / 3d, 95.0);
  }
}

void OrderBook() {
  std::mt19937_64 rng(2);
  std::uniform_int_distribution<int> tick(-50, 50);
  std::uniform_int_distribution<int> qty(-100, 200);
  hanson::ch04::OrderBook book;
  for (int i = 0; i < 200'000; ++i) {
    const int t = tick(rng);
    book.Update(t < 0 ? hanson::ch04::Side::kBid : hanson::ch04::Side::kAsk,
                100.0 + 0.01 * t, qty(rng));
    if (i % 100 == 0) {
      g_sink += static_cast<double>(
          book.ExecuteMarket(hanson::ch04::Side::kBid, 50).size());
    }
  }
}

struct Workload {
  std::string_view name;
  void (*fn)();
};

constexpr Workload kWorkloads[] = {
    {"stats", Stats},
    {"mc", MonteCarlo},
    {"mc_parallel", MonteCarloParallel},
    {"matrix", Matrix},
    {"tree", Tree},
    {"bonds", Bonds},
    {"orderbook", OrderBook},
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::puts("usage: hanson_profile <workload|all>");
    for (const auto& w : kWorkloads)
      std::printf("  %.*s\n", static_cast<int>(w.name.size()), w.name.data());
    return 1;
  }
  const std::string_view want = argv[1];
  bool ran = false;
  for (const auto& w : kWorkloads) {
    if (want == "all" || want == w.name) {
      w.fn();
      ran = true;
    }
  }
  std::printf("sink %.6f\n", g_sink);
  return ran ? 0 : 1;
}
