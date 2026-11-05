#include "ch12_situation_2007/modern.h"

#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch12 {

std::vector<double> GenerateTerminalSpots(const BsInputs& in, std::size_t n,
                                          Rng& rng) {
  const double var = in.vol * in.vol * in.expiry;
  const double moved = in.spot * std::exp((in.r - in.d) * in.expiry - 0.5 * var);
  const double sd = std::sqrt(var);
  std::normal_distribution<double> normal;
  std::vector<double> spots(n);
  for (double& s : spots) s = moved * std::exp(sd * normal(rng));
  return spots;
}

double MeanPayOffVirtual(const ch04::PayOff& payoff,
                         std::span<const double> spots) {
  double sum = 0.0;
  for (const double s : spots) sum += payoff(s);
  return sum / static_cast<double>(spots.size());
}

double MeanPayOffVirtualBatched(const ch04::PayOff& payoff,
                                std::span<const double> spots) {
  constexpr std::size_t kBlock = 256;
  double values[kBlock];
  double sum = 0.0;
  for (std::size_t start = 0; start < spots.size(); start += kBlock) {
    const std::size_t n = std::min(kBlock, spots.size() - start);
    payoff.Evaluate(spots.subspan(start, n), std::span<double>(values, n));
    for (std::size_t i = 0; i < n; ++i) sum += values[i];
  }
  return sum / static_cast<double>(spots.size());
}

double MeanPayOffFunction(const std::function<double(double)>& payoff,
                          std::span<const double> spots) {
  double sum = 0.0;
  for (const double s : spots) sum += payoff(s);
  return sum / static_cast<double>(spots.size());
}

double MeanPayOffVariant(const PayOffVariant& payoff,
                         std::span<const double> spots) {
  double sum = 0.0;
  for (const double s : spots) {
    sum += std::visit([s](const auto& p) { return p(s); }, payoff);
  }
  return sum / static_cast<double>(spots.size());
}

SharedVanillaOption::SharedVanillaOption(
    std::shared_ptr<const ch04::PayOff> payoff, double expiry)
    : payoff_(std::move(payoff)), expiry_(expiry) {
  if (!payoff_) throw std::invalid_argument("SharedVanillaOption: null");
}

}  // namespace cpp4q::joshi::ch12
