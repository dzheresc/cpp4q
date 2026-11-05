// Chapter 12: More Sophisticated Classes.
//
// Inheritance beyond interfaces: an abstract base that holds data
// (protected members, a constructor that validates them), the template-
// method pattern (a final Payoff() in the base calling private virtual
// hooks the subclasses fill in), final classes, and operator overloading
// for output. The contracts are the classic path-dependent ones: barrier
// options (up/down, in/out) and Asian options (arithmetic and geometric
// averages), with the closed forms used to test them.

#ifndef CPP4Q_ARMSTRONG_CH12_SOPHISTICATED_CLASSES_PATH_DEPENDENT_H_
#define CPP4Q_ARMSTRONG_CH12_SOPHISTICATED_CLASSES_PATH_DEPENDENT_H_

#include <iosfwd>
#include <span>

#include "ch10_interfaces/interfaces.h"

namespace cpp4q::armstrong::ch12 {

using ch10::BlackScholesModel;
using ch10::ContinuousTimeOption;
using ch10::Estimate;

enum class OptionType { kCall, kPut };

class PathDependentOption : public ContinuousTimeOption {
 public:
  double maturity() const final { return maturity_; }
  bool IsPathDependent() const final { return true; }

 protected:
  explicit PathDependentOption(double maturity);  // throws if < 0
  double maturity_;
};

// Pays the vanilla payoff at K if the barrier was (knock-in) or was not
// (knock-out) touched on any monitoring date.
class BarrierOption : public PathDependentOption {
 public:
  double Payoff(std::span<const double> prices) const final;
  double strike() const { return strike_; }
  double barrier() const { return barrier_; }

 protected:
  // Throws unless strike > 0 and barrier > 0.
  BarrierOption(double strike, double barrier, double maturity);
  double strike_;
  double barrier_;

 private:
  virtual bool Breached(std::span<const double> prices) const = 0;
  virtual bool IsKnockIn() const = 0;
  virtual double VanillaPayoff(double s) const = 0;
};

class UpAndOutCall final : public BarrierOption {
 public:
  UpAndOutCall(double strike, double barrier, double maturity)
      : BarrierOption(strike, barrier, maturity) {}

 private:
  bool Breached(std::span<const double> prices) const override;
  bool IsKnockIn() const override { return false; }
  double VanillaPayoff(double s) const override;
};

class UpAndInCall final : public BarrierOption {
 public:
  UpAndInCall(double strike, double barrier, double maturity)
      : BarrierOption(strike, barrier, maturity) {}

 private:
  bool Breached(std::span<const double> prices) const override;
  bool IsKnockIn() const override { return true; }
  double VanillaPayoff(double s) const override;
};

class DownAndOutPut final : public BarrierOption {
 public:
  DownAndOutPut(double strike, double barrier, double maturity)
      : BarrierOption(strike, barrier, maturity) {}

 private:
  bool Breached(std::span<const double> prices) const override;
  bool IsKnockIn() const override { return false; }
  double VanillaPayoff(double s) const override;
};

class DownAndInPut final : public BarrierOption {
 public:
  DownAndInPut(double strike, double barrier, double maturity)
      : BarrierOption(strike, barrier, maturity) {}

 private:
  bool Breached(std::span<const double> prices) const override;
  bool IsKnockIn() const override { return true; }
  double VanillaPayoff(double s) const override;
};

// Pays (A - K)^+ or (K - A)^+ for the average A of the monitored prices.
class AsianOption : public PathDependentOption {
 public:
  double Payoff(std::span<const double> prices) const final;
  double strike() const { return strike_; }
  OptionType type() const { return type_; }

 protected:
  AsianOption(OptionType type, double strike, double maturity);
  OptionType type_;
  double strike_;

 private:
  virtual double Average(std::span<const double> prices) const = 0;
};

class ArithmeticAsianOption final : public AsianOption {
 public:
  ArithmeticAsianOption(OptionType type, double strike, double maturity)
      : AsianOption(type, strike, maturity) {}

 private:
  double Average(std::span<const double> prices) const override;
};

class GeometricAsianOption final : public AsianOption {
 public:
  GeometricAsianOption(OptionType type, double strike, double maturity)
      : AsianOption(type, strike, maturity) {}

 private:
  double Average(std::span<const double> prices) const override;
};

// Closed form for the geometric Asian call on n equally spaced fixings
// date + i tau / n, i = 1..n (log of the average is normal).
double GeometricAsianCallPrice(const BlackScholesModel& m, double strike,
                               double maturity, int fixings);

// Continuously monitored up-and-in / up-and-out calls for barrier H > K
// (Merton / Reiner-Rubinstein). For H <= K the up-and-in equals the call.
double UpAndInCallPrice(const BlackScholesModel& m, double strike,
                        double barrier, double maturity);
double UpAndOutCallPrice(const BlackScholesModel& m, double strike,
                         double barrier, double maturity);

// Broadie-Glasserman-Kou: a barrier monitored at m dates prices like a
// continuous one at H exp(+-0.5826 sigma sqrt(dt)) (+ for up barriers).
double DiscreteBarrierShift(double barrier, double vol, double dt, bool up);

}  // namespace cpp4q::armstrong::ch12

namespace cpp4q::armstrong::ch09 {

// "price +- std_error". Declared in Estimate's own namespace so that
// argument-dependent lookup finds it from anywhere.
std::ostream& operator<<(std::ostream& out, const Estimate& e);

}  // namespace cpp4q::armstrong::ch09

#endif  // CPP4Q_ARMSTRONG_CH12_SOPHISTICATED_CLASSES_PATH_DEPENDENT_H_
