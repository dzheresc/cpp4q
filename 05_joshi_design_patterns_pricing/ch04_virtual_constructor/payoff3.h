// Chapter 4: Bridging with a virtual constructor (pay-offs).
//
// A VanillaOption has to own its pay-off, and copying an option has to
// copy the pay-off without knowing its dynamic type. The fix is the virtual
// constructor: every PayOff can Clone() itself. The book returns a raw
// `PayOff*` from Clone; here it returns std::unique_ptr<PayOff>, so
// ownership is in the type and nothing can leak.

#ifndef CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF3_H_
#define CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF3_H_

#include <cstddef>
#include <memory>
#include <span>

namespace cpp4q::joshi::ch04 {

class PayOff {
 public:
  virtual ~PayOff() = default;
  virtual double operator()(double spot) const = 0;
  virtual std::unique_ptr<PayOff> Clone() const = 0;
  // values[i] = (*this)(spots[i]): one virtual call for a whole batch
  // instead of one per spot. The default loops over operator(); Cloneable
  // overrides it with a loop the compiler can inline and vectorise.
  virtual void Evaluate(std::span<const double> spots,
                        std::span<double> values) const {
    for (std::size_t i = 0; i < spots.size(); ++i) {
      values[i] = (*this)(spots[i]);
    }
  }

 protected:
  PayOff() = default;
  PayOff(const PayOff&) = default;
  PayOff& operator=(const PayOff&) = default;
};

// CRTP helper: implements Clone() for any copyable Derived, so concrete
// pay-offs do not each repeat the same one-liner, and Evaluate() with a
// statically bound (inlinable) call to Derived::operator().
template <class Derived, class Base = PayOff>
class Cloneable : public Base {
 public:
  std::unique_ptr<Base> Clone() const override {
    return std::make_unique<Derived>(static_cast<const Derived&>(*this));
  }
  void Evaluate(std::span<const double> spots,
                std::span<double> values) const override {
    const auto& self = static_cast<const Derived&>(*this);
    for (std::size_t i = 0; i < spots.size(); ++i) {
      values[i] = self.Derived::operator()(spots[i]);
    }
  }
};

class PayOffCall final : public Cloneable<PayOffCall> {
 public:
  explicit PayOffCall(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

class PayOffPut final : public Cloneable<PayOffPut> {
 public:
  explicit PayOffPut(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

// Pays 1 if S > strike.
class PayOffDigitalCall final : public Cloneable<PayOffDigitalCall> {
 public:
  explicit PayOffDigitalCall(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

// Pays 1 if lower <= S <= upper.
class PayOffDoubleDigital final : public Cloneable<PayOffDoubleDigital> {
 public:
  PayOffDoubleDigital(double lower, double upper)
      : lower_(lower), upper_(upper) {}
  double operator()(double spot) const override;

 private:
  double lower_;
  double upper_;
};

}  // namespace cpp4q::joshi::ch04

#endif  // CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF3_H_
