// Chapter 3: composition with smart pointers.
//
// Option *owns* its payoff (unique_ptr, deep-copied via Clone) and *shares*
// a read-only yield curve (shared_ptr<const>) with other instruments.

#ifndef CPP4Q_HANSON_CH03_POLYMORPHISM_OPTION_H_
#define CPP4Q_HANSON_CH03_POLYMORPHISM_OPTION_H_

#include <memory>

#include "ch03_polymorphism/payoff.h"
#include "ch03_polymorphism/yield_curve.h"

namespace cpp4q::hanson::ch03 {

class Option {
 public:
  // Throws std::invalid_argument on null payoff/curve or negative expiry.
  Option(std::unique_ptr<Payoff> payoff, double expiry,
         std::shared_ptr<const YieldCurve> curve);

  Option(const Option& other);
  Option& operator=(const Option& other);
  Option(Option&&) noexcept = default;
  Option& operator=(Option&&) noexcept = default;
  ~Option() = default;

  [[nodiscard]] double expiry() const { return expiry_; }
  [[nodiscard]] double PayoffAt(double spot) const { return (*payoff_)(spot); }
  // Discounted payoff for a given terminal spot.
  [[nodiscard]] double DiscountedPayoff(double spot) const;
  [[nodiscard]] const YieldCurve& curve() const { return *curve_; }
  [[nodiscard]] long curve_use_count() const { return curve_.use_count(); }

 private:
  std::unique_ptr<Payoff> payoff_;
  double expiry_;
  std::shared_ptr<const YieldCurve> curve_;
};

}  // namespace cpp4q::hanson::ch03

#endif  // CPP4Q_HANSON_CH03_POLYMORPHISM_OPTION_H_
