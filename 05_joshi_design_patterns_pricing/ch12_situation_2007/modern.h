// Chapter 12: The situation in 2007 (and since).
//
// The book takes stock of what the language and its libraries offered a
// quant developer in 2007: TR1/Boost shared_ptr and function, templates
// instead of virtual functions where speed matters, and the libraries
// built on them. Everything it looked forward to is now standard C++, so
// this chapter puts the four ways of passing a pay-off to a pricer side by
// side, each in its C++20 form:
//
//   dynamic    virtual call through ch04::PayOff (the book's choice), per
//              spot or per block of spots
//   erased     std::function<double(double)> (TR1 function in 2007)
//   static     a template constrained by the PayOffLike concept
//   closed     std::variant of the known pay-offs, dispatched by std::visit
//
// plus shared_ptr<const PayOff> for sharing an immutable pay-off instead of
// cloning it. They all compute the same number; bench/ measures the cost
// of each dispatch on a fixed set of terminal spots.

#ifndef CPP4Q_JOSHI_CH12_SITUATION_2007_MODERN_H_
#define CPP4Q_JOSHI_CH12_SITUATION_2007_MODERN_H_

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <variant>
#include <vector>

#include "ch01_simple_monte_carlo/simple_mc.h"
#include "ch04_virtual_constructor/payoff3.h"

namespace cpp4q::joshi::ch12 {

struct BsInputs {
  double spot = 100.0;
  double r = 0.05;
  double d = 0.0;
  double vol = 0.2;
  double expiry = 1.0;
};

// Terminal spots of `n` exact Black-Scholes paths.
std::vector<double> GenerateTerminalSpots(const BsInputs& in, std::size_t n,
                                          Rng& rng);

template <class F>
concept PayOffLike = std::regular_invocable<const F&, double> &&
                     std::convertible_to<std::invoke_result_t<const F&, double>,
                                         double>;

// Value types, no base class: usable by the template and variant pricers.
struct Call {
  double strike;
  double operator()(double s) const { return std::max(s - strike, 0.0); }
};
struct Put {
  double strike;
  double operator()(double s) const { return std::max(strike - s, 0.0); }
};
struct DoubleDigital {
  double lower;
  double upper;
  double operator()(double s) const {
    return s >= lower && s <= upper ? 1.0 : 0.0;
  }
};
using PayOffVariant = std::variant<Call, Put, DoubleDigital>;

// Mean pay-off over `spots`, by each dispatch mechanism.
double MeanPayOffVirtual(const ch04::PayOff& payoff,
                         std::span<const double> spots);
// Still dynamic, but one virtual call per block of spots
// (ch04::PayOff::Evaluate), so the per-spot work inlines.
double MeanPayOffVirtualBatched(const ch04::PayOff& payoff,
                                std::span<const double> spots);
double MeanPayOffFunction(const std::function<double(double)>& payoff,
                          std::span<const double> spots);
double MeanPayOffVariant(const PayOffVariant& payoff,
                         std::span<const double> spots);
template <PayOffLike F>
double MeanPayOffTemplate(const F& payoff, std::span<const double> spots) {
  double sum = 0.0;
  for (const double s : spots) sum += payoff(s);
  return sum / static_cast<double>(spots.size());
}

// One immutable pay-off shared by many options: copies are reference
// counted rather than cloned. Safe because nothing can modify it.
class SharedVanillaOption {
 public:
  SharedVanillaOption(std::shared_ptr<const ch04::PayOff> payoff,
                      double expiry);
  double expiry() const { return expiry_; }
  double OptionPayOff(double spot) const { return (*payoff_)(spot); }
  long use_count() const { return payoff_.use_count(); }

 private:
  std::shared_ptr<const ch04::PayOff> payoff_;
  double expiry_;
};

}  // namespace cpp4q::joshi::ch12

#endif  // CPP4Q_JOSHI_CH12_SITUATION_2007_MODERN_H_
