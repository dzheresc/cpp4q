#include "ch04_virtual_constructor/parameters.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch04 {

Parameters::Parameters(const ParametersInner& inner) : inner_(inner.Clone()) {}

Parameters::Parameters(double constant)
    : inner_(std::make_unique<ParametersConstant>(constant)) {}

Parameters::Parameters(const Parameters& other)
    : inner_(other.inner_->Clone()) {}

Parameters& Parameters::operator=(const Parameters& other) {
  inner_ = other.inner_->Clone();
  return *this;
}

double Parameters::Mean(double t1, double t2) const {
  return Integral(t1, t2) / (t2 - t1);
}

double Parameters::RootMeanSquare(double t1, double t2) const {
  return std::sqrt(IntegralSquare(t1, t2) / (t2 - t1));
}

std::unique_ptr<ParametersInner> ParametersConstant::Clone() const {
  return std::make_unique<ParametersConstant>(*this);
}

double ParametersConstant::Integral(double t1, double t2) const {
  return (t2 - t1) * constant_;
}

double ParametersConstant::IntegralSquare(double t1, double t2) const {
  return (t2 - t1) * constant_ * constant_;
}

ParametersPiecewiseConstant::ParametersPiecewiseConstant(
    std::vector<double> times, std::vector<double> values)
    : times_(std::move(times)), values_(std::move(values)) {
  if (values_.size() != times_.size() + 1) {
    throw std::invalid_argument(
        "ParametersPiecewiseConstant: need one more value than times");
  }
  double previous = 0.0;
  for (const double t : times_) {
    if (!(t > previous)) {
      throw std::invalid_argument(
          "ParametersPiecewiseConstant: times must increase from 0");
    }
    previous = t;
  }
}

std::unique_ptr<ParametersInner> ParametersPiecewiseConstant::Clone() const {
  return std::make_unique<ParametersPiecewiseConstant>(*this);
}

template <class F>
double ParametersPiecewiseConstant::IntegrateFrom0(double t, F f) const {
  double sum = 0.0;
  double start = 0.0;
  std::size_t i = 0;
  for (; i < times_.size() && times_[i] < t; ++i) {
    sum += f(values_[i]) * (times_[i] - start);
    start = times_[i];
  }
  return sum + f(values_[i]) * (t - start);
}

double ParametersPiecewiseConstant::Integral(double t1, double t2) const {
  const auto id = [](double v) { return v; };
  return IntegrateFrom0(t2, id) - IntegrateFrom0(t1, id);
}

double ParametersPiecewiseConstant::IntegralSquare(double t1,
                                                   double t2) const {
  const auto sq = [](double v) { return v * v; };
  return IntegrateFrom0(t2, sq) - IntegrateFrom0(t1, sq);
}

}  // namespace cpp4q::joshi::ch04
