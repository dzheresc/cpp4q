// Chapter 3: an abstract yield curve shared between instruments through
// std::shared_ptr<const YieldCurve>.

#ifndef CPP4Q_HANSON_CH03_POLYMORPHISM_YIELD_CURVE_H_
#define CPP4Q_HANSON_CH03_POLYMORPHISM_YIELD_CURVE_H_

#include <vector>

namespace cpp4q::hanson::ch03 {

class YieldCurve {
 public:
  virtual ~YieldCurve() = default;
  // Continuously compounded zero rate for maturity t (years).
  [[nodiscard]] virtual double ZeroRate(double t) const = 0;
  [[nodiscard]] double DiscountFactor(double t) const;
  [[nodiscard]] double ForwardRate(double t1, double t2) const;
};

class FlatYieldCurve final : public YieldCurve {
 public:
  explicit FlatYieldCurve(double rate) : rate_(rate) {}
  double ZeroRate(double) const override { return rate_; }

 private:
  double rate_;
};

// Zero rates linearly interpolated between pillars, flat extrapolation.
class LinearZeroCurve final : public YieldCurve {
 public:
  // Throws std::invalid_argument unless times are strictly increasing and
  // sizes match and are non-zero.
  LinearZeroCurve(std::vector<double> times, std::vector<double> rates);
  double ZeroRate(double t) const override;

 private:
  std::vector<double> times_;
  std::vector<double> rates_;
};

}  // namespace cpp4q::hanson::ch03

#endif  // CPP4Q_HANSON_CH03_POLYMORPHISM_YIELD_CURVE_H_
