// Fixed, deterministic workloads for valgrind (memcheck, helgrind, drd,
// callgrind, cachegrind, massif). Sizes are small because valgrind runs
// 20-100x slower than native.
// Usage: glasserman_profile <workload>; run without arguments to list.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string_view>
#include <vector>

#include "ch01_foundations/monte_carlo.h"
#include "ch02_random_numbers/distributions.h"
#include "ch02_random_numbers/normal.h"
#include "ch02_random_numbers/parallel_mc.h"
#include "ch02_random_numbers/rng.h"
#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/short_rate.h"
#include "ch04_variance_reduction/variance_reduction.h"
#include "ch05_quasi_monte_carlo/low_discrepancy.h"
#include "ch05_quasi_monte_carlo/qmc_pricing.h"
#include "ch06_discretization/schemes.h"
#include "ch07_sensitivities/greeks.h"
#include "ch08_american_options/lattice.h"
#include "ch08_american_options/lsm.h"
#include "ch08_american_options/random_tree.h"
#include "ch09_risk_management/credit.h"
#include "ch09_risk_management/portfolio_var.h"

namespace {

namespace g = cpp4q::glasserman;
using g::ch01::OptionType;
using g::ch02::Mrg32k3a;
using g::ch02::Sampler;

double g_sink = 0.0;  // keeps results observable

void Uniforms() {
  Mrg32k3a rng(1);
  for (int i = 0; i < 1'000'000; ++i) g_sink += rng.NextUniform();
}

void Normals() {
  for (auto m :
       {g::ch02::NormalMethod::kInversion, g::ch02::NormalMethod::kBoxMuller,
        g::ch02::NormalMethod::kMarsagliaPolar}) {
    Sampler s(Mrg32k3a(2), m);
    for (int i = 0; i < 200'000; ++i) g_sink += s.Normal();
  }
}

void Variates() {
  Sampler s(Mrg32k3a(3));
  for (int i = 0; i < 50'000; ++i) {
    g_sink += g::ch02::SampleGamma(s, 2.5);
    g_sink += g::ch02::SampleNoncentralChiSquare(s, 1.6, 30.0);
    g_sink += static_cast<double>(g::ch02::SamplePoisson(s, 4.0));
  }
  std::vector<double> cov = {1.0, 0.5, 0.2, 0.5, 2.0, 0.3, 0.2, 0.3, 1.5};
  g::ch02::MultivariateNormal mvn({0, 0, 0}, cov);
  double out[3];
  for (int i = 0; i < 20'000; ++i) {
    mvn.Sample(s, out);
    g_sink += out[2];
  }
}

void Parallel() {
  auto path = [](Sampler& s) {
    return std::max(100.0 * std::exp(0.03 + 0.2 * s.Normal()) - 100.0, 0.0);
  };
  g_sink += g::ch02::ParallelSimulate(path, 200'000,
                                      {.threads = 4, .block_size = 4096})
                .Mean();
}

void Bridge() {
  const g::ch03::BrownianBridge bb(g::ch03::UniformGrid(1.0, 64));
  Sampler s(Mrg32k3a(4));
  std::vector<double> z(64), w(64);
  for (int i = 0; i < 20'000; ++i) {
    s.FillNormal(z);
    bb.Build(z, w);
    g_sink += w[63];
  }
}

void ShortRate() {
  Sampler s(Mrg32k3a(5));
  const g::ch03::ShortRateParams p{.kappa = 0.3, .theta = 0.05, .sigma = 0.1};
  g_sink += g::ch03::VasicekBondMc(p, 0.03, 5.0, 20, 10'000, s).mean;
  g_sink += g::ch03::CirBondMc(p, 0.03, 5.0, 20, 10'000, s).mean;
}

void VarianceReduction() {
  Sampler s(Mrg32k3a(6));
  const g::ch01::BlackScholesInputs in{};
  const g::ch04::AsianSpec asian{in, 12};
  g_sink += g::ch04::PlainCall(in, 20'000, s).mean;
  g_sink += g::ch04::AntitheticCall(in, 10'000, s).mean;
  g_sink += g::ch04::ControlVariateCall(in, 20'000, s).estimate.mean;
  g_sink += g::ch04::StratifiedCall(in, 100, 200, s).mean;
  g_sink += g::ch04::ImportanceSamplingCall(in, 1.0, 20'000, s).mean;
  g_sink += g::ch04::PlainAsian(asian, 20'000, s).mean;
  g_sink += g::ch04::GeometricControlAsian(asian, 20'000, s).estimate.mean;
  g_sink += g::ch04::TerminalStratifiedAsian(asian, 100, 200, s).mean;
}

void Qmc() {
  g_sink += g::ch05::QmcAsianCall(
      {}, 16'384, g::ch05::PathConstruction::kBrownianBridge, 3);
  g_sink +=
      g::ch05::QmcAsianCall({}, 16'384, g::ch05::PathConstruction::kRandomWalk);
  g::ch05::Halton h(8);
  std::vector<double> p(8);
  for (int i = 0; i < 20'000; ++i) {
    h.Next(p);
    g_sink += p[7];
  }
}

void Discrepancy() {
  constexpr std::size_t kDim = 4, kN = 512;
  g::ch05::Sobol sobol(kDim);
  std::vector<double> pts(kN * kDim);
  for (std::size_t i = 0; i < kN; ++i) {
    sobol.Next(std::span(pts).subspan(i * kDim, kDim));
  }
  g_sink += g::ch05::L2StarDiscrepancy(pts, kDim);
}

void Discretization() {
  const g::ch03::GbmParams gbm{100, 0.05, 0.4};
  g_sink +=
      g::ch06::GbmStrongError(gbm, 1, 64, g::ch06::Scheme::kEuler, 2000, 1)
          .mean;
  g_sink +=
      g::ch06::GbmStrongError(gbm, 1, 64, g::ch06::Scheme::kMilstein, 2000, 1)
          .mean;
  g_sink += g::ch06::RichardsonCallPrice({}, 16, 5000, 2).mean;
  const g::ch03::ShortRateParams cir{.kappa = 0.2, .theta = 0.04, .sigma = 0.3};
  g_sink += g::ch06::CirEulerBond(cir, 0.02, 5, 20,
                                  g::ch06::CirFix::kFullTruncation, 5000, 3)
                .mean;
}

void Greeks() {
  g_sink += g::ch07::EuropeanCallGreeks({}, 50'000, 1).lr_vega.mean;
  g_sink += g::ch07::DigitalCallGreeks({}, 50'000, 2).lr_delta.mean;
  g_sink += g::ch07::AsianCallDelta({}, 12, 10'000, 3).pathwise_delta.mean;
  g_sink += g::ch07::FiniteDifferenceDelta(
                {}, 0.01, g::ch07::FiniteDifference::kCentral, true, 50'000, 4)
                .mean;
}

const g::ch01::BlackScholesInputs kPut{
    .spot = 100, .strike = 110, .rate = 0.06, .vol = 0.25, .maturity = 1};

void Lsm() {
  const g::ch08::BermudanSpec spec{kPut, OptionType::kPut, 50};
  g::ch08::LsmPricer lsm(spec, 4);
  g_sink += lsm.Train(20'000, 1).mean;
  g_sink += lsm.Price(20'000, 2).mean;
  g_sink += g::ch08::CrrPrice(kPut, OptionType::kPut, 1000,
                              g::ch08::ExerciseStyle::kAmerican);
}

void Tree() {
  const g::ch08::BermudanSpec spec{kPut, OptionType::kPut, 3};
  g_sink += g::ch08::RandomTreePrice(spec, 20, 5, 1).low.mean;
}

g::ch09::OptionPortfolio Book() {
  g::ch09::Market m{
      .spots = {100.0, 50.0, 200.0},
      .vols = {0.3, 0.25, 0.2},
      .correlation = {1.0, 0.5, 0.3, 0.5, 1.0, 0.4, 0.3, 0.4, 1.0},
      .rate = 0.04};
  return g::ch09::OptionPortfolio(m,
                                  {{0, OptionType::kCall, 100.0, 0.5, -10.0},
                                   {0, OptionType::kPut, 95.0, 0.5, -10.0},
                                   {1, OptionType::kCall, 55.0, 0.25, -20.0},
                                   {2, OptionType::kPut, 190.0, 1.0, 5.0},
                                   {2, OptionType::kCall, 210.0, 1.0, -15.0}});
}

void Var() {
  const auto book = Book();
  const auto losses =
      g::ch09::SimulateLosses(book, 10.0 / 252, 20'000, false, 1);
  g_sink += g::ch09::ValueAtRisk(losses, 0.01);
  g_sink += g::ch09::TailProbabilityIs(book, 10.0 / 252, 330.0, 5'000, 2).mean;
}

void Credit() {
  std::vector<g::ch09::Obligor> obligors;
  for (int k = 0; k < 100; ++k) {
    obligors.push_back(
        {0.01 * (1 + k % 5), 1.0 + (k % 3), 0.3 + 0.05 * (k % 4)});
  }
  const g::ch09::GaussianCopulaPortfolio book(obligors);
  const double shift = book.FindFactorShift(40.0);
  g_sink += book.TailProbability(40.0, 2'000, 1).mean;
  g_sink += book.TailProbabilityIs(40.0, shift, 2'000, 2).mean;
}

struct Workload {
  const char* name;
  void (*run)();
};

constexpr Workload kWorkloads[] = {
    {"uniforms", Uniforms},
    {"normals", Normals},
    {"variates", Variates},
    {"parallel", Parallel},
    {"bridge", Bridge},
    {"shortrate", ShortRate},
    {"vr", VarianceReduction},
    {"qmc", Qmc},
    {"discrepancy", Discrepancy},
    {"discretization", Discretization},
    {"greeks", Greeks},
    {"lsm", Lsm},
    {"tree", Tree},
    {"var", Var},
    {"credit", Credit},
};

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::printf("usage: %s <workload>\nworkloads:", argv[0]);
    for (const auto& w : kWorkloads) std::printf(" %s", w.name);
    std::printf("\n");
    return 1;
  }
  for (const auto& w : kWorkloads) {
    if (std::string_view(argv[1]) == w.name) {
      w.run();
      std::printf("%s: %g\n", w.name, g_sink);
      return 0;
    }
  }
  std::fprintf(stderr, "unknown workload %s\n", argv[1]);
  return 1;
}
