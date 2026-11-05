// Chapter 2: Writing User-Defined Classes with Modern C++ Features.
//
// A value-semantic option class: invariants enforced in the constructor,
// const accessors, [[nodiscard]], defaulted comparison and a hidden-friend
// stream operator.

#ifndef CPP4Q_HANSON_CH02_CLASSES_EUROPEAN_OPTION_H_
#define CPP4Q_HANSON_CH02_CLASSES_EUROPEAN_OPTION_H_

#include <compare>
#include <ostream>

#include "ch01_overview/black_scholes.h"

namespace cpp4q::hanson::ch02 {

using ch01::OptionType;

struct MarketData {
  double spot = 0.0;
  double rate = 0.0;
  double dividend_yield = 0.0;
  double volatility = 0.0;
};

class EuropeanOption {
 public:
  // Throws std::invalid_argument if strike <= 0 or expiry < 0.
  EuropeanOption(OptionType type, double strike, double time_to_expiry);

  [[nodiscard]] OptionType type() const { return type_; }
  [[nodiscard]] double strike() const { return strike_; }
  [[nodiscard]] double time_to_expiry() const { return time_to_expiry_; }

  [[nodiscard]] double Price(const MarketData& market) const;
  [[nodiscard]] ch01::Greeks Greeks(const MarketData& market) const;
  [[nodiscard]] double Payoff(double spot_at_expiry) const;

  // Ages the option by `dt` years (expiry floors at zero).
  void Advance(double dt);

  // Member-wise ordering: type, strike, expiry. Lets options be sorted or
  // used as std::map keys without writing six operators by hand.
  friend auto operator<=>(const EuropeanOption&,
                          const EuropeanOption&) = default;

  friend std::ostream& operator<<(std::ostream& os, const EuropeanOption& o);

 private:
  ch01::BlackScholesInputs ToInputs(const MarketData& market) const;

  OptionType type_;
  double strike_;
  double time_to_expiry_;
};

}  // namespace cpp4q::hanson::ch02

#endif  // CPP4Q_HANSON_CH02_CLASSES_EUROPEAN_OPTION_H_
