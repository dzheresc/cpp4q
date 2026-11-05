// Fixed, deterministic workloads for valgrind (memcheck, helgrind, drd,
// callgrind, cachegrind, massif). Sizes are small because valgrind runs
// 20-100x slower than native.
// Usage: armstrong_profile <workload>; run without arguments to list.

#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <string_view>
#include <vector>

#include "appendix_a_risk_neutral/binomial.h"
#include "ch03_functions/matlib.h"
#include "ch05_multiple_files/vectors.h"
#include "ch09_monte_carlo/monte_carlo.h"
#include "ch12_sophisticated_classes/path_dependent.h"
#include "ch13_portfolio/portfolio.h"
#include "ch14_delta_hedging/hedging.h"
#include "ch16_matrix/matrix.h"
#include "ch16_matrix/multi_stock.h"
#include "ch18_stl/stl.h"
#include "ch19_function_objects/solvers.h"
#include "ch20_threads/parallel_pricer.h"
#include "ch21_next_steps/pde.h"

namespace {

namespace a = cpp4q::armstrong;

double g_sink = 0.0;  // keeps results observable

void Normals() {
  for (int i = 1; i < 200'000; ++i) {
    const double p = i / 200'000.0;
    g_sink += a::ch03::NormInv(p) + a::ch03::NormCdf(p - 0.5) +
              a::ch03::NormCdfPolynomial(p);
  }
}

void Mc09() {
  const a::ch08::BlackScholesModel m;
  a::ch09::Rng rng(1);
  g_sink += a::ch09::MonteCarloPricer(100'000)
                .Price(a::ch08::CallOption(100, 1), m, rng)
                .price;
}

void Asian() {
  const a::ch10::BlackScholesModel m;
  a::ch10::Rng rng(2);
  const a::ch12::ArithmeticAsianOption asian(a::ch12::OptionType::kCall, 100,
                                             1);
  g_sink += a::ch10::MonteCarloPricer(10'000, 52).Price(asian, m, rng).price;
}

void Barrier() {
  const a::ch10::BlackScholesModel m;
  a::ch10::Rng rng(3);
  g_sink += a::ch10::MonteCarloPricer(10'000, 52)
                .Price(a::ch12::UpAndOutCall(100, 130, 1), m, rng)
                .price;
}

void Hedging() {
  const a::ch14::BlackScholesModel world;
  a::ch14::Rng rng(4);
  const a::ch14::HedgingSimulator sim(world, world,
                                      a::ch14::EuropeanCall(100, 1), 100);
  for (double x : sim.RunSimulations(1000, rng)) g_sink += x;
}

void PortfolioGreeks() {
  const a::ch13::BlackScholesModel m;
  a::ch13::Portfolio p;
  for (int i = 0; i < 200; ++i) {
    p.Add(1, std::make_shared<a::ch10::EuropeanCall>(80 + 0.2 * i, 1));
    p.Add(-1, std::make_shared<a::ch10::EuropeanPut>(80 + 0.2 * i, 1));
  }
  for (int k = 0; k < 20; ++k) {
    g_sink += p.Price(m) + p.Delta(m) + p.Gamma(m) + p.Vega(m);
  }
}

a::ch16::Matrix RandomMatrix(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(-1, 1);
  a::ch16::Matrix m(n, n);
  for (std::size_t k = 0; k < m.size(); ++k) m.data()[k] = u(rng);
  return m;
}

void Matmul() {
  const auto x = RandomMatrix(200, 1), y = RandomMatrix(200, 2);
  g_sink += (x * y)(7, 9);
}

void Linalg() {
  const auto x = RandomMatrix(120, 3);
  const auto spd = x * x.Transpose() + a::ch16::Matrix::Identity(120);
  g_sink += a::ch16::Cholesky(spd)(119, 119);
  const std::vector<double> b(120, 1.0);
  g_sink += a::ch16::Solve(x, b)[0];
}

void Basket() {
  constexpr std::size_t kN = 10;
  a::ch16::Matrix rho(kN, kN, 0.3);
  for (std::size_t i = 0; i < kN; ++i) rho(i, i) = 1;
  const a::ch16::MultiStockModel model(std::vector<double>(kN, 100),
                                       std::vector<double>(kN, 0.2), rho, 0.05);
  const std::vector<double> w(kN, 0.1);
  a::ch16::Rng rng(5);
  g_sink += a::ch16::PriceBasketCall(model, w, 100, 1, 20'000, rng).price;
}

void Callables() {
  const auto f = [](double x) { return x * x * std::exp(-x); };
  const std::function<double(double)> sf = f;
  for (int i = 0; i < 5; ++i) {
    g_sink += a::ch19::IntegrateTemplate(f, 0, 10, 50'000);
    g_sink += a::ch19::IntegrateStdFunction(sf, 0, 10, 50'000);
    g_sink += a::ch10::Integrate(a::ch19::FunctionAdapter(f), 0, 10, 50'000);
  }
}

void ImpliedVol() {
  for (int i = 0; i < 2000; ++i) {
    const double k = 60 + 0.04 * i;
    const double price = a::ch03::BlackScholesCallPrice(k, 1, 100, 0.25, 0.03);
    g_sink += a::ch19::ImpliedVolatility(price, k, 1, 100, 0.03);
  }
}

void Parallel() {
  const a::ch20::BlackScholesModel m;
  const a::ch12::GeometricAsianOption geo(a::ch12::OptionType::kCall, 100, 1);
  a::ch20::ParallelSettings s{.n_scenarios = 20'000, .n_steps = 12,
                              .n_threads = 4, .n_blocks = 16};
  g_sink += a::ch20::PriceParallel(geo, m, s).price;
  g_sink += a::ch20::PriceWithAsync(geo, m, s).price;
  g_sink += a::ch20::PriceWithMutex(geo, m, s).price;
}

void Var() {
  std::mt19937_64 rng(6);
  std::student_t_distribution<double> t(4);
  std::vector<double> losses(200'000);
  for (double& x : losses) x = t(rng);
  g_sink += a::ch18::HistoricalVaR(losses, 0.99) +
            a::ch18::HistoricalExpectedShortfall(losses, 0.99) +
            a::ch05::Prctile(losses, 99);
}

void PdeEuropean() {
  g_sink += a::ch21::PricePde(a::ch21::PayoffType::kPut,
                              a::ch21::ExerciseStyle::kEuropean, 100, 1, 100,
                              0.2, 0.05, {.n_space = 400, .n_time = 400})
                .price;
}

void PdeAmerican() {
  g_sink += a::ch21::PricePde(a::ch21::PayoffType::kPut,
                              a::ch21::ExerciseStyle::kAmerican, 100, 1, 100,
                              0.2, 0.05, {.n_space = 200, .n_time = 200})
                .price;
}

void Crr() {
  g_sink += a::appendix_a::CrrPrice(a::appendix_a::PayoffType::kPut,
                                    a::appendix_a::ExerciseStyle::kAmerican,
                                    100, 1, 100, 0.2, 0.05, 1000);
}

struct Workload {
  std::string_view name;
  void (*run)();
};

constexpr Workload kWorkloads[] = {
    {"normals", Normals},     {"mc09", Mc09},
    {"asian", Asian},         {"barrier", Barrier},
    {"hedging", Hedging},     {"portfolio", PortfolioGreeks},
    {"matmul", Matmul},       {"linalg", Linalg},
    {"basket", Basket},       {"callables", Callables},
    {"impliedvol", ImpliedVol}, {"parallel", Parallel},
    {"var", Var},             {"pde_eu", PdeEuropean},
    {"pde_am", PdeAmerican},  {"crr", Crr},
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: armstrong_profile <workload>\nworkloads:");
    for (const auto& w : kWorkloads) {
      std::printf(" %.*s", static_cast<int>(w.name.size()), w.name.data());
    }
    std::printf("\n");
    return 1;
  }
  for (const auto& w : kWorkloads) {
    if (w.name == argv[1]) {
      w.run();
      std::printf("%s: %.6g\n", argv[1], g_sink);
      return 0;
    }
  }
  std::fprintf(stderr, "unknown workload %s\n", argv[1]);
  return 1;
}
