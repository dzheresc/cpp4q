#include "ch07_exotics_engine/exotic_engine.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch07 {

ExoticEngine::ExoticEngine(Wrapper<PathDependent> product, ch04::Parameters r)
    : product_(std::move(product)), r_(std::move(r)) {
  if (!product_) throw std::invalid_argument("ExoticEngine: no product");
  these_cash_flows_.resize(product_->MaxNumberOfCashFlows());
  for (const double t : product_->PossibleCashFlowTimes()) {
    discounts_.push_back(std::exp(-r_.Integral(0.0, t)));
  }
}

void ExoticEngine::DoSimulation(ch05::StatisticsMc& gatherer,
                                std::uint64_t num_paths) {
  std::vector<double> spot_values(product_->LookAtTimes().size());
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    GetOnePath(spot_values);
    gatherer.DumpOneResult(DoOnePath(spot_values));
  }
}

double ExoticEngine::DoOnePath(std::span<const double> spot_values) const {
  const std::size_t n = product_->CashFlows(spot_values, these_cash_flows_);
  double value = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    value += these_cash_flows_[i].amount *
             discounts_[these_cash_flows_[i].time_index];
  }
  return value;
}

ExoticBsEngine::ExoticBsEngine(Wrapper<PathDependent> product,
                               ch04::Parameters r, ch04::Parameters d,
                               ch04::Parameters vol,
                               Wrapper<ch06::RandomBase> generator,
                               double spot)
    : ExoticEngine(std::move(product), r),
      generator_(std::move(generator)),
      log_spot_(std::log(spot)) {
  if (!generator_) throw std::invalid_argument("ExoticBsEngine: no generator");
  const std::vector<double>& times = this->product().LookAtTimes();
  const std::size_t n = times.size();
  generator_->ResetDimensionality(n);
  variates_.resize(n);
  drifts_.resize(n);
  standard_deviations_.resize(n);
  double previous = 0.0;
  for (std::size_t j = 0; j < n; ++j) {
    const double var = vol.IntegralSquare(previous, times[j]);
    drifts_[j] = r.Integral(previous, times[j]) -
                 d.Integral(previous, times[j]) - 0.5 * var;
    standard_deviations_[j] = std::sqrt(var);
    previous = times[j];
  }
}

void ExoticBsEngine::GetOnePath(std::span<double> spot_values) {
  generator_->GetGaussians(variates_);
  double current_log_spot = log_spot_;
  for (std::size_t j = 0; j < spot_values.size(); ++j) {
    current_log_spot += drifts_[j] + standard_deviations_[j] * variates_[j];
    spot_values[j] = std::exp(current_log_spot);
  }
}

}  // namespace cpp4q::joshi::ch07
