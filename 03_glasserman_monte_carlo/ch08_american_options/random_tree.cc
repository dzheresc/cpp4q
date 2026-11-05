#include "ch08_american_options/random_tree.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch08 {
namespace {

struct NodeValue {
  double high;
  double low;
};

class TreeBuilder {
 public:
  TreeBuilder(const BermudanSpec& spec, std::size_t branching,
              ch02::Sampler& sampler)
      : spec_(spec), b_(branching), sampler_(sampler) {
    const auto& mk = spec.market;
    const double dt = mk.maturity / static_cast<double>(spec.exercise_dates);
    drift_ = (mk.rate - mk.dividend - 0.5 * mk.vol * mk.vol) * dt;
    sd_ = mk.vol * std::sqrt(dt);
    disc_ = std::exp(-mk.rate * dt);
  }

  // Value at a node at exercise date `date` (0 = today, not exercisable).
  NodeValue Value(double spot, std::size_t date) {
    const double h = date == 0 ? 0.0 : Payoff(spot);
    if (date == spec_.exercise_dates) return {h, h};
    std::vector<NodeValue> children(b_);
    double high_sum = 0.0;
    double low_sum = 0.0;
    for (auto& c : children) {
      c = Value(spot * std::exp(drift_ + sd_ * sampler_.Normal()), date + 1);
      high_sum += c.high;
      low_sum += c.low;
    }
    const auto b = static_cast<double>(b_);
    const double high = std::max(h, disc_ * high_sum / b);
    double low = 0.0;
    for (const auto& c : children) {
      const double others = disc_ * (low_sum - c.low) / (b - 1.0);
      low += (date > 0 && h >= others) ? h : disc_ * c.low;
    }
    return {high, low / b};
  }

 private:
  double Payoff(double s) const {
    const double k = spec_.market.strike;
    return spec_.type == ch01::OptionType::kPut ? std::max(k - s, 0.0)
                                                : std::max(s - k, 0.0);
  }

  const BermudanSpec& spec_;
  std::size_t b_;
  ch02::Sampler& sampler_;
  double drift_ = 0.0;
  double sd_ = 0.0;
  double disc_ = 1.0;
};

}  // namespace

RandomTreeResult RandomTreePrice(const BermudanSpec& spec,
                                 std::size_t branching, std::size_t trees,
                                 std::uint64_t seed) {
  if (branching < 2) throw std::invalid_argument("branching must be >= 2");
  ch02::Sampler sampler{ch02::Mrg32k3a(seed)};
  TreeBuilder builder(spec, branching, sampler);
  ch01::RunningStats high, low;
  for (std::size_t t = 0; t < trees; ++t) {
    const NodeValue v = builder.Value(spec.market.spot, 0);
    high.Add(v.high);
    low.Add(v.low);
  }
  return {high.ToEstimate(), low.ToEstimate()};
}

}  // namespace cpp4q::glasserman::ch08
