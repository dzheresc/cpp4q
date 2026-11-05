// Chapter 9 demo: VaR of an option book by full revaluation and by
// delta-gamma, importance sampling for a tail probability, and a credit
// portfolio tail with Glasserman-Li importance sampling.

#include <cstdio>
#include <vector>

#include "ch09_risk_management/credit.h"
#include "ch09_risk_management/portfolio_var.h"

namespace g = cpp4q::glasserman;
using g::ch01::OptionType;

int main() {
  g::ch09::Market m{
      .spots = {100.0, 50.0, 200.0},
      .vols = {0.3, 0.25, 0.2},
      .correlation = {1.0, 0.5, 0.3, 0.5, 1.0, 0.4, 0.3, 0.4, 1.0},
      .rate = 0.04};
  const g::ch09::OptionPortfolio book(
      m, {{0, OptionType::kCall, 100.0, 0.5, -10.0},
          {0, OptionType::kPut, 95.0, 0.5, -10.0},
          {1, OptionType::kCall, 55.0, 0.25, -20.0},
          {2, OptionType::kPut, 190.0, 1.0, 5.0},
          {2, OptionType::kCall, 210.0, 1.0, -15.0}});
  const double horizon = 10.0 / 252.0;
  const auto full = g::ch09::SimulateLosses(book, horizon, 100'000, false, 1);
  const auto dg = g::ch09::SimulateLosses(book, horizon, 100'000, true, 1);
  std::printf("10-day 99%% VaR: full revaluation %.2f, delta-gamma %.2f\n",
              g::ch09::ValueAtRisk(full, 0.01), g::ch09::ValueAtRisk(dg, 0.01));
  std::printf("10-day 99%% ES:  full revaluation %.2f, delta-gamma %.2f\n",
              g::ch09::ExpectedShortfall(full, 0.01),
              g::ch09::ExpectedShortfall(dg, 0.01));

  const double x = g::ch09::ValueAtRisk(full, 0.001);
  const auto plain = g::ch09::TailProbability(book, horizon, x, 20'000, 2);
  const auto mean_shift = g::ch09::TailProbabilityIs(
      book, horizon, x, 20'000, 2, g::ch09::IsMethod::kMeanShift);
  const auto twist = g::ch09::TailProbabilityIs(
      book, horizon, x, 20'000, 2, g::ch09::IsMethod::kDeltaGammaTwist);
  std::printf("P(L > %.2f), 20k paths each:\n", x);
  std::printf("  plain               %.6f +- %.6f\n", plain.mean,
              plain.HalfWidth());
  std::printf("  IS, mean shift      %.6f +- %.6f\n", mean_shift.mean,
              mean_shift.HalfWidth());
  std::printf("  IS, delta-gamma     %.6f +- %.6f\n", twist.mean,
              twist.HalfWidth());

  std::vector<g::ch09::Obligor> obligors;
  for (int k = 0; k < 100; ++k) {
    obligors.push_back(
        {0.01 * (1 + k % 5), 1.0 + (k % 3), 0.3 + 0.05 * (k % 4)});
  }
  const g::ch09::GaussianCopulaPortfolio credit(obligors);
  const double threshold = 40.0;
  const double shift = credit.FindFactorShift(threshold);
  const auto c_plain = credit.TailProbability(threshold, 20'000, 3);
  const auto c_is = credit.TailProbabilityIs(threshold, shift, 20'000, 3);
  std::printf("\nCredit book: expected loss %.2f, factor shift %.2f\n",
              credit.ExpectedLoss(), shift);
  std::printf("P(L > %.0f): plain %.2e +- %.2e, IS %.3e +- %.2e\n", threshold,
              c_plain.mean, c_plain.HalfWidth(), c_is.mean, c_is.HalfWidth());
  return 0;
}
