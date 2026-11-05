// Chapter 4: time-dependent parameters through the same bridge.
//
// Volatility and the short rate enter a terminal-value Monte Carlo only
// through their integral and the integral of their square over [t1, t2].
// ParametersInner is the polymorphic interface (with a virtual
// constructor); Parameters is its value-semantic bridge with the derived
// quantities Mean and RootMeanSquare. A constant and a piecewise-constant
// implementation are provided.

#ifndef CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PARAMETERS_H_
#define CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PARAMETERS_H_

#include <memory>
#include <vector>

namespace cpp4q::joshi::ch04 {

class ParametersInner {
 public:
  virtual ~ParametersInner() = default;
  virtual std::unique_ptr<ParametersInner> Clone() const = 0;
  virtual double Integral(double t1, double t2) const = 0;
  virtual double IntegralSquare(double t1, double t2) const = 0;

 protected:
  ParametersInner() = default;
  ParametersInner(const ParametersInner&) = default;
  ParametersInner& operator=(const ParametersInner&) = default;
};

class Parameters {
 public:
  Parameters(const ParametersInner& inner);  // NOLINT: implicit by design
  // A constant parameter: Parameters p = 0.2;
  Parameters(double constant);  // NOLINT: implicit by design

  Parameters(const Parameters& other);
  Parameters& operator=(const Parameters& other);
  Parameters(Parameters&&) noexcept = default;
  Parameters& operator=(Parameters&&) noexcept = default;
  ~Parameters() = default;

  double Integral(double t1, double t2) const {
    return inner_->Integral(t1, t2);
  }
  double IntegralSquare(double t1, double t2) const {
    return inner_->IntegralSquare(t1, t2);
  }
  double Mean(double t1, double t2) const;
  double RootMeanSquare(double t1, double t2) const;

 private:
  std::unique_ptr<ParametersInner> inner_;
};

class ParametersConstant final : public ParametersInner {
 public:
  explicit ParametersConstant(double constant) : constant_(constant) {}
  std::unique_ptr<ParametersInner> Clone() const override;
  double Integral(double t1, double t2) const override;
  double IntegralSquare(double t1, double t2) const override;

 private:
  double constant_;
};

// value[i] on [times[i-1], times[i]) with times[-1] = 0; the last value
// extends to infinity. Throws unless times are strictly increasing and
// positive and there is one more value than times.
class ParametersPiecewiseConstant final : public ParametersInner {
 public:
  ParametersPiecewiseConstant(std::vector<double> times,
                              std::vector<double> values);
  std::unique_ptr<ParametersInner> Clone() const override;
  double Integral(double t1, double t2) const override;
  double IntegralSquare(double t1, double t2) const override;

 private:
  template <class F>
  double IntegrateFrom0(double t, F f) const;

  std::vector<double> times_;
  std::vector<double> values_;
};

}  // namespace cpp4q::joshi::ch04

#endif  // CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PARAMETERS_H_
