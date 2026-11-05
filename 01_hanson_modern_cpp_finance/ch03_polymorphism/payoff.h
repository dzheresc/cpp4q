// Chapter 3: Inheritance, Polymorphism, and Smart Pointers.
//
// A classic abstract Payoff hierarchy with a virtual Clone() returning
// std::unique_ptr, plus a closed-set std::variant alternative so the two
// dispatch styles can be benchmarked against each other.

#ifndef CPP4Q_HANSON_CH03_POLYMORPHISM_PAYOFF_H_
#define CPP4Q_HANSON_CH03_POLYMORPHISM_PAYOFF_H_

#include <memory>
#include <variant>

namespace cpp4q::hanson::ch03 {

class Payoff {
 public:
  // Appendix A: a polymorphic base needs a virtual destructor, otherwise
  // deleting a derived object through Payoff* is undefined behaviour.
  virtual ~Payoff() = default;

  [[nodiscard]] virtual double operator()(double spot) const = 0;
  [[nodiscard]] virtual std::unique_ptr<Payoff> Clone() const = 0;

 protected:
  // Protected copy prevents slicing via `Payoff p = call;` (Appendix B) while
  // still letting derived classes be copied.
  Payoff() = default;
  Payoff(const Payoff&) = default;
  Payoff& operator=(const Payoff&) = default;
};

class CallPayoff final : public Payoff {
 public:
  explicit CallPayoff(double strike) : strike_(strike) {}
  double operator()(double spot) const override;
  std::unique_ptr<Payoff> Clone() const override;
  [[nodiscard]] double strike() const { return strike_; }

 private:
  double strike_;
};

class PutPayoff final : public Payoff {
 public:
  explicit PutPayoff(double strike) : strike_(strike) {}
  double operator()(double spot) const override;
  std::unique_ptr<Payoff> Clone() const override;

 private:
  double strike_;
};

// Pays `cash` if spot > strike.
class DigitalCallPayoff final : public Payoff {
 public:
  DigitalCallPayoff(double strike, double cash)
      : strike_(strike), cash_(cash) {}
  double operator()(double spot) const override;
  std::unique_ptr<Payoff> Clone() const override;

 private:
  double strike_;
  double cash_;
};

// Pays 1 if lower <= spot <= upper.
class DoubleDigitalPayoff final : public Payoff {
 public:
  DoubleDigitalPayoff(double lower, double upper);
  double operator()(double spot) const override;
  std::unique_ptr<Payoff> Clone() const override;

 private:
  double lower_;
  double upper_;
};

// --- Closed-set alternative: value semantics, no heap, inlinable. ---------

struct Call {
  double strike;
  [[nodiscard]] double operator()(double s) const {
    return s > strike ? s - strike : 0.0;
  }
};
struct Put {
  double strike;
  [[nodiscard]] double operator()(double s) const {
    return s < strike ? strike - s : 0.0;
  }
};
struct DigitalCall {
  double strike;
  double cash;
  [[nodiscard]] double operator()(double s) const {
    return s > strike ? cash : 0.0;
  }
};

using PayoffVariant = std::variant<Call, Put, DigitalCall>;

inline double Evaluate(const PayoffVariant& payoff, double spot) {
  return std::visit([spot](const auto& p) { return p(spot); }, payoff);
}

}  // namespace cpp4q::hanson::ch03

#endif  // CPP4Q_HANSON_CH03_POLYMORPHISM_PAYOFF_H_
