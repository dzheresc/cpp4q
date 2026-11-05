// Chapter 3: Inheritance and virtual functions.
//
// PayOff is now an abstract interface and each contract is a class derived
// from it. The Monte Carlo routine takes `const PayOff&` and is never edited
// again when a contract is added: the open-closed principle. The double
// digital is the proof: it was written after SimpleMonteCarlo3 and needed
// no change to it.
//
// The base has a virtual destructor (deleting through a base pointer is
// well defined) and its copy operations are protected, so a PayOff cannot
// be sliced by accidental copying through the base.

#ifndef CPP4Q_JOSHI_CH03_INHERITANCE_PAYOFF2_H_
#define CPP4Q_JOSHI_CH03_INHERITANCE_PAYOFF2_H_

#include <cstdint>

#include "ch01_simple_monte_carlo/simple_mc.h"

namespace cpp4q::joshi::ch03 {

class PayOff {
 public:
  virtual ~PayOff() = default;
  virtual double operator()(double spot) const = 0;

 protected:
  PayOff() = default;
  PayOff(const PayOff&) = default;
  PayOff& operator=(const PayOff&) = default;
};

class PayOffCall final : public PayOff {
 public:
  explicit PayOffCall(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

class PayOffPut final : public PayOff {
 public:
  explicit PayOffPut(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

// Pays 1 if lower <= S(T) <= upper.
class PayOffDoubleDigital final : public PayOff {
 public:
  PayOffDoubleDigital(double lower, double upper)
      : lower_(lower), upper_(upper) {}
  double operator()(double spot) const override;

 private:
  double lower_;
  double upper_;
};

McResult SimpleMonteCarlo3(const PayOff& payoff, double expiry, double spot,
                           double vol, double r, std::uint64_t num_paths,
                           Rng& rng);

}  // namespace cpp4q::joshi::ch03

#endif  // CPP4Q_JOSHI_CH03_INHERITANCE_PAYOFF2_H_
