#include "ch06_discretization/schemes.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch06 {

ch01::Estimate GbmStrongError(const ch03::GbmParams& p, double maturity,
                              std::size_t steps, Scheme scheme,
                              std::size_t paths, std::uint64_t seed) {
  if (steps == 0) throw std::invalid_argument("steps must be > 0");
  const double dt = maturity / static_cast<double>(steps);
  const double sqrt_dt = std::sqrt(dt);
  auto a = [&p](double, double x) { return p.drift * x; };
  auto b = [&p](double, double x) { return p.vol * x; };
  auto db = [&p](double, double) { return p.vol; };
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    double x = p.spot;
    double w = 0.0;
    double t = 0.0;
    for (std::size_t k = 0; k < steps; ++k) {
      const double dw = sqrt_dt * s.Normal();
      x = scheme == Scheme::kEuler ? EulerStep(a, b, t, x, dt, dw)
                                   : MilsteinStep(a, b, db, t, x, dt, dw);
      w += dw;
      t += dt;
    }
    const double exact =
        p.spot *
        std::exp((p.drift - 0.5 * p.vol * p.vol) * maturity + p.vol * w);
    st.Add(std::abs(x - exact));
  }
  return st.ToEstimate();
}

double FitOrder(std::span<const std::size_t> steps,
                std::span<const double> errors) {
  if (steps.size() != errors.size() || steps.size() < 2) {
    throw std::invalid_argument("need matching inputs, at least two");
  }
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (std::size_t i = 0; i < steps.size(); ++i) {
    const double x = -std::log(static_cast<double>(steps[i]));  // log h + c
    const double y = std::log(errors[i]);
    sx += x;
    sy += y;
    sxx += x * x;
    sxy += x * y;
  }
  const auto n = static_cast<double>(steps.size());
  return (n * sxy - sx * sy) / (n * sxx - sx * sx);
}

double EulerGbmMean(const ch03::GbmParams& p, double maturity,
                    std::size_t steps) {
  const double h = maturity / static_cast<double>(steps);
  return p.spot * std::pow(1.0 + p.drift * h, static_cast<double>(steps));
}

double EulerGbmSecondMoment(const ch03::GbmParams& p, double maturity,
                            std::size_t steps) {
  const double h = maturity / static_cast<double>(steps);
  const double g = 1.0 + p.drift * h;
  return p.spot * p.spot *
         std::pow(g * g + p.vol * p.vol * h, static_cast<double>(steps));
}

ch01::Estimate EulerCallPrice(const ch01::BlackScholesInputs& in,
                              std::size_t steps, std::size_t paths,
                              bool two_point, std::uint64_t seed) {
  if (steps == 0) throw std::invalid_argument("steps must be > 0");
  const double dt = in.maturity / static_cast<double>(steps);
  const double sqrt_dt = std::sqrt(dt);
  const double mu = in.rate - in.dividend;
  const double df = std::exp(-in.rate * in.maturity);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    double x = in.spot;
    for (std::size_t k = 0; k < steps; ++k) {
      const double dw = two_point ? (s.Uniform() < 0.5 ? -sqrt_dt : sqrt_dt)
                                  : sqrt_dt * s.Normal();
      x += mu * x * dt + in.vol * x * dw;
    }
    st.Add(df * std::max(x - in.strike, 0.0));
  }
  return st.ToEstimate();
}

ch01::Estimate RichardsonCallPrice(const ch01::BlackScholesInputs& in,
                                   std::size_t coarse_steps, std::size_t paths,
                                   std::uint64_t seed) {
  if (coarse_steps == 0) throw std::invalid_argument("steps must be > 0");
  const double h = in.maturity / static_cast<double>(coarse_steps);
  const double sqrt_half = std::sqrt(0.5 * h);
  const double mu = in.rate - in.dividend;
  const double df = std::exp(-in.rate * in.maturity);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    double coarse = in.spot;
    double fine = in.spot;
    for (std::size_t k = 0; k < coarse_steps; ++k) {
      const double dw1 = sqrt_half * s.Normal();
      const double dw2 = sqrt_half * s.Normal();
      fine += mu * fine * 0.5 * h + in.vol * fine * dw1;
      fine += mu * fine * 0.5 * h + in.vol * fine * dw2;
      coarse += mu * coarse * h + in.vol * coarse * (dw1 + dw2);
    }
    st.Add(df * (2.0 * std::max(fine - in.strike, 0.0) -
                 std::max(coarse - in.strike, 0.0)));
  }
  return st.ToEstimate();
}

ch01::Estimate CirEulerBond(const ch03::ShortRateParams& p, double r0,
                            double maturity, std::size_t steps, CirFix fix,
                            std::size_t paths, std::uint64_t seed) {
  if (steps == 0) throw std::invalid_argument("steps must be > 0");
  const double dt = maturity / static_cast<double>(steps);
  const double sqrt_dt = std::sqrt(dt);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    double r = r0;
    double integral = 0.0;
    for (std::size_t k = 0; k < steps; ++k) {
      const double z = s.Normal();
      double next;
      switch (fix) {
        case CirFix::kAbsorb:
          next = r + p.kappa * (p.theta - r) * dt +
                 p.sigma * std::sqrt(r) * sqrt_dt * z;
          next = std::max(next, 0.0);
          break;
        case CirFix::kReflect:
          next = std::abs(r + p.kappa * (p.theta - r) * dt +
                          p.sigma * std::sqrt(r) * sqrt_dt * z);
          break;
        case CirFix::kFullTruncation:
        default: {
          // Lord, Koekkoek and van Dijk: let r go negative, use r+ in the
          // coefficients; the rate used for discounting is r+.
          const double rp = std::max(r, 0.0);
          next = r + p.kappa * (p.theta - rp) * dt +
                 p.sigma * std::sqrt(rp) * sqrt_dt * z;
          break;
        }
      }
      integral += 0.5 * (std::max(r, 0.0) + std::max(next, 0.0)) * dt;
      r = next;
    }
    st.Add(std::exp(-integral));
  }
  return st.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch06
