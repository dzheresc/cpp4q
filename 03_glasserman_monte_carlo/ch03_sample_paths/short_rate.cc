#include "ch03_sample_paths/short_rate.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ch02_random_numbers/distributions.h"

namespace cpp4q::glasserman::ch03 {

double VasicekBondPrice(const ShortRateParams& p, double r0, double t) {
  const double k = p.kappa;
  const double b = (1.0 - std::exp(-k * t)) / k;
  const double log_a =
      (b - t) * (k * k * p.theta - 0.5 * p.sigma * p.sigma) / (k * k) -
      p.sigma * p.sigma * b * b / (4.0 * k);
  return std::exp(log_a - b * r0);
}

VasicekExactStep::VasicekExactStep(const ShortRateParams& p, double dt)
    : p_(p), decay_(std::exp(-p.kappa * dt)), dt_(dt) {
  const double k = p.kappa;
  const double s2 = p.sigma * p.sigma;
  int_coef_ = (1.0 - decay_) / k;
  const double var_r = s2 / (2.0 * k) * (1.0 - decay_ * decay_);
  const double var_y =
      s2 / (k * k) *
      (dt + (1.0 - decay_ * decay_) / (2.0 * k) - 2.0 * (1.0 - decay_) / k);
  const double cov = s2 / (2.0 * k * k) * (1.0 - decay_) * (1.0 - decay_);
  sd_r_ = std::sqrt(var_r);
  beta_ = cov / sd_r_;
  sd_y_cond_ = std::sqrt(std::max(var_y - beta_ * beta_, 0.0));
}

RateAndIntegral VasicekExactStep::Step(double r, double z1, double z2) const {
  const double gap = r - p_.theta;
  const double rate = p_.theta + gap * decay_ + sd_r_ * z1;
  const double integral =
      p_.theta * dt_ + gap * int_coef_ + beta_ * z1 + sd_y_cond_ * z2;
  return {rate, integral};
}

ch01::Estimate VasicekBondMc(const ShortRateParams& p, double r0,
                             double maturity, std::size_t steps,
                             std::size_t paths, ch02::Sampler& s) {
  const VasicekExactStep step(p, maturity / static_cast<double>(steps));
  ch01::RunningStats stats;
  for (std::size_t i = 0; i < paths; ++i) {
    double r = r0;
    double integral = 0.0;
    for (std::size_t k = 0; k < steps; ++k) {
      const double z1 = s.Normal();
      const double z2 = s.Normal();
      const RateAndIntegral next = step.Step(r, z1, z2);
      r = next.rate;
      integral += next.integral;
    }
    stats.Add(std::exp(-integral));
  }
  return stats.ToEstimate();
}

double CirBondPrice(const ShortRateParams& p, double r0, double t) {
  const double k = p.kappa;
  const double h = std::sqrt(k * k + 2.0 * p.sigma * p.sigma);
  const double e = std::exp(h * t) - 1.0;
  const double denom = 2.0 * h + (k + h) * e;
  const double a = std::pow(2.0 * h * std::exp(0.5 * (k + h) * t) / denom,
                            2.0 * k * p.theta / (p.sigma * p.sigma));
  const double b = 2.0 * e / denom;
  return a * std::exp(-b * r0);
}

double CirExactStep(const ShortRateParams& p, double r, double dt,
                    ch02::Sampler& s) {
  const double decay = std::exp(-p.kappa * dt);
  const double c = p.sigma * p.sigma * (1.0 - decay) / (4.0 * p.kappa);
  const double dof = 4.0 * p.kappa * p.theta / (p.sigma * p.sigma);
  const double lambda = r * decay / c;
  return c * ch02::SampleNoncentralChiSquare(s, dof, lambda);
}

ch01::Estimate CirBondMc(const ShortRateParams& p, double r0, double maturity,
                         std::size_t steps, std::size_t paths,
                         ch02::Sampler& s) {
  if (steps == 0) throw std::invalid_argument("steps must be > 0");
  const double dt = maturity / static_cast<double>(steps);
  ch01::RunningStats stats;
  for (std::size_t i = 0; i < paths; ++i) {
    double r = r0;
    double integral = 0.0;
    for (std::size_t k = 0; k < steps; ++k) {
      const double next = CirExactStep(p, r, dt, s);
      integral += 0.5 * (r + next) * dt;
      r = next;
    }
    stats.Add(std::exp(-integral));
  }
  return stats.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch03
