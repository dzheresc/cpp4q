#include "ch16_decoupling/mc_pricer.h"

#include <cmath>
#include <utility>

#include "ch04_virtual_constructor/payoff_bridge.h"
#include "ch04_virtual_constructor/vanilla.h"
#include "ch05_strategies_statistics/statistics.h"
#include "ch06_random_numbers/random.h"
#include "ch14_template_factory/registrations.h"

namespace cpp4q::joshi::ch16 {

class McPricer::Impl {
 public:
  Impl(ch04::PayOffBridge payoff, double expiry)
      : option_(std::move(payoff), expiry) {}

  ch04::VanillaOption option_;
  double spot_ = 100.0;
  double r_ = 0.0;
  double d_ = 0.0;
  double vol_ = 0.2;
  std::string generator_name_ = "park_miller";
};

McPricer::McPricer(const std::string& payoff_name, double strike,
                   double expiry)
    : impl_(std::make_unique<Impl>(
          ch04::PayOffBridge(ch14::StrikePayOffFactory::Instance().Create(
              payoff_name, strike)),
          expiry)) {}

McPricer::McPricer(const ch04::PayOff& payoff, double expiry)
    : impl_(std::make_unique<Impl>(ch04::PayOffBridge(payoff), expiry)) {}

// Defined here, where Impl is complete: unique_ptr<Impl>'s deleter needs it.
McPricer::~McPricer() = default;
McPricer::McPricer(McPricer&&) noexcept = default;
McPricer& McPricer::operator=(McPricer&&) noexcept = default;

McPricer::McPricer(const McPricer& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}

McPricer& McPricer::operator=(const McPricer& other) {
  if (this != &other) impl_ = std::make_unique<Impl>(*other.impl_);
  return *this;
}

void McPricer::SetMarket(double spot, double r, double d, double vol) {
  impl_->spot_ = spot;
  impl_->r_ = r;
  impl_->d_ = d;
  impl_->vol_ = vol;
}

void McPricer::SetGenerator(const std::string& generator_name) {
  if (!ch14::RandomFactory::Instance().Contains(generator_name)) {
    // Fail now rather than at the next Price().
    ch14::RandomFactory::Instance().Create(generator_name, 1, 1);
  }
  impl_->generator_name_ = generator_name;
}

PricingResult McPricer::Price(std::uint64_t num_paths,
                              std::uint64_t seed) const {
  auto generator =
      ch14::RandomFactory::Instance().Create(impl_->generator_name_, 1, seed);
  ch05::StatisticsMoments stats;
  // A stock with dividend yield d has the same terminal distribution as
  // one paying none with spot S e^{-dT}.
  ch06::SimpleMonteCarlo6(impl_->option_, impl_->spot_ *
                              std::exp(-impl_->d_ * impl_->option_.expiry()),
                          impl_->vol_, impl_->r_, num_paths, stats, *generator);
  const auto row = stats.GetResultsSoFar()[0];
  return {row[0], row[1]};
}

std::vector<std::string> McPricer::KnownPayOffs() {
  return ch14::StrikePayOffFactory::Instance().Keys();
}

}  // namespace cpp4q::joshi::ch16
