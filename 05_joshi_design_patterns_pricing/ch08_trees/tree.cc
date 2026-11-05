#include "ch08_trees/tree.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cpp4q::joshi::ch08 {

void TreeProduct::PreFinalValues(std::span<const double> spots, double time,
                                 std::span<double> values,
                                 std::span<double> /*scratch*/) const {
  for (std::size_t k = 0; k < spots.size(); ++k) {
    values[k] = PreFinalValue(spots[k], time, values[k]);
  }
}

void TreeProduct::FinalPayOffs(std::span<const double> spots,
                               std::span<double> values) const {
  for (std::size_t k = 0; k < spots.size(); ++k) {
    values[k] = FinalPayOff(spots[k]);
  }
}

void TreeEuropean::PreFinalValues(std::span<const double> /*spots*/,
                                  double /*time*/,
                                  std::span<double> /*values*/,
                                  std::span<double> /*scratch*/) const {}

void TreeEuropean::FinalPayOffs(std::span<const double> spots,
                                std::span<double> values) const {
  payoff_.get().Evaluate(spots, values);
}

void TreeAmerican::PreFinalValues(std::span<const double> spots,
                                  double /*time*/, std::span<double> values,
                                  std::span<double> scratch) const {
  payoff_.get().Evaluate(spots, scratch);
  for (std::size_t k = 0; k < spots.size(); ++k) {
    values[k] = std::max(scratch[k], values[k]);
  }
}

void TreeAmerican::FinalPayOffs(std::span<const double> spots,
                                std::span<double> values) const {
  payoff_.get().Evaluate(spots, values);
}

double TreeEuropean::PreFinalValue(double /*spot*/, double /*time*/,
                                   double discounted_future_value) const {
  return discounted_future_value;
}

std::unique_ptr<TreeProduct> TreeEuropean::Clone() const {
  return std::make_unique<TreeEuropean>(*this);
}

double TreeAmerican::PreFinalValue(double spot, double /*time*/,
                                   double discounted_future_value) const {
  return std::max(payoff_(spot), discounted_future_value);
}

std::unique_ptr<TreeProduct> TreeAmerican::Clone() const {
  return std::make_unique<TreeAmerican>(*this);
}

SimpleBinomialTree::SimpleBinomialTree(double spot, ch04::Parameters r,
                                       ch04::Parameters d, double vol,
                                       std::size_t steps, double time)
    : spot_(spot),
      r_(std::move(r)),
      d_(std::move(d)),
      vol_(vol),
      steps_(steps),
      time_(time) {
  if (steps_ == 0 || !(time_ > 0.0)) {
    throw std::invalid_argument("SimpleBinomialTree: need steps, time > 0");
  }
}

void SimpleBinomialTree::BuildTree() {
  tree_built_ = true;
  const double initial_log_spot = std::log(spot_);
  const double dt = time_ / static_cast<double>(steps_);
  const double sd = vol_ * std::sqrt(dt);
  const double up = std::exp(2.0 * sd);
  up_pow_.resize(steps_ + 1);
  // Powers by repeated multiplication drift by ~k ulps; exp(2 k sd) every
  // 64 steps keeps them within a few ulps at a sixty-fourth of the cost.
  for (std::size_t k = 0; k <= steps_; ++k) {
    up_pow_[k] = k % 64 == 0
                     ? std::exp(2.0 * sd * static_cast<double>(k))
                     : up_pow_[k - 1] * up;
  }
  lowest_spot_.resize(steps_ + 1);
  for (std::size_t i = 0; i <= steps_; ++i) {
    const double t = static_cast<double>(i) * dt;
    const double moved_log_spot = initial_log_spot + r_.Integral(0, t) -
                                  d_.Integral(0, t) - 0.5 * vol_ * vol_ * t;
    lowest_spot_[i] =
        std::exp(moved_log_spot - static_cast<double>(i) * sd);
  }
  discounts_.resize(steps_);
  for (std::size_t i = 0; i < steps_; ++i) {
    discounts_[i] = std::exp(-r_.Integral(static_cast<double>(i) * dt,
                                          static_cast<double>(i + 1) * dt));
  }
  spots_.resize(steps_ + 1);
  values_.resize(steps_ + 2);
  scratch_.resize(steps_ + 1);
}

double SimpleBinomialTree::GetThePrice(const TreeProduct& product) {
  if (!tree_built_) BuildTree();
  if (std::fabs(product.final_time() - time_) > 1e-12 * time_) {
    throw std::invalid_argument("SimpleBinomialTree: mismatched final time");
  }
  const auto layer_spots = [this](std::size_t i) {
    for (std::size_t k = 0; k <= i; ++k) {
      spots_[k] = lowest_spot_[i] * up_pow_[k];
    }
    return std::span<const double>(spots_.data(), i + 1);
  };
  product.FinalPayOffs(layer_spots(steps_),
                       std::span<double>(values_.data(), steps_ + 1));
  const double dt = time_ / static_cast<double>(steps_);
  for (std::size_t i = steps_; i-- > 0;) {
    const double half_discount = 0.5 * discounts_[i];
    // In place: values_[k] is overwritten only after its last read.
    for (std::size_t k = 0; k <= i; ++k) {
      values_[k] = half_discount * (values_[k] + values_[k + 1]);
    }
    product.PreFinalValues(layer_spots(i), static_cast<double>(i) * dt,
                           std::span<double>(values_.data(), i + 1),
                           std::span<double>(scratch_.data(), i + 1));
  }
  return values_[0];
}

}  // namespace cpp4q::joshi::ch08
