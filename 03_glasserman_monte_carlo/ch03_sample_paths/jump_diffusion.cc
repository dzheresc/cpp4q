#include "ch03_sample_paths/jump_diffusion.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ch01_foundations/black_scholes.h"
#include "ch02_random_numbers/distributions.h"

namespace cpp4q::glasserman::ch03 {
namespace {

// E[jump factor] - 1.
double MeanJump(const MertonParams& p) {
  return std::exp(p.jump_mean + 0.5 * p.jump_vol * p.jump_vol) - 1.0;
}

}  // namespace

double MertonCallPrice(const MertonParams& p, double strike, double maturity,
                       int terms) {
  const double k = MeanJump(p);
  const double lambda_prime = p.jump_intensity * (1.0 + k);
  const double mean_jumps = lambda_prime * maturity;
  double weight = std::exp(-mean_jumps);  // Poisson(n; lambda' T)
  double price = 0.0;
  for (int n = 0; n < terms; ++n) {
    if (n > 0) weight *= mean_jumps / n;
    const double vol_n =
        std::sqrt(p.vol * p.vol + n * p.jump_vol * p.jump_vol / maturity);
    const double rate_n =
        p.rate - p.jump_intensity * k + n * std::log(1.0 + k) / maturity;
    const ch01::BlackScholesInputs in{.spot = p.spot,
                                      .strike = strike,
                                      .rate = rate_n,
                                      .vol = vol_n,
                                      .maturity = maturity,
                                      .dividend = p.dividend};
    // Weighting by Poisson(lambda' T) rather than Poisson(lambda T) absorbs
    // the change of discount rate from r_n back to r.
    price += weight * ch01::BlackScholesPrice(ch01::OptionType::kCall, in);
  }
  return price;
}

void MertonPath(const MertonParams& p, std::span<const double> times,
                ch02::Sampler& s, std::span<double> out) {
  if (out.size() != times.size()) throw std::invalid_argument("size mismatch");
  const double mu = p.rate - p.dividend - p.jump_intensity * MeanJump(p) -
                    0.5 * p.vol * p.vol;
  double x = std::log(p.spot);
  double prev_t = 0.0;
  for (std::size_t i = 0; i < times.size(); ++i) {
    const double dt = times[i] - prev_t;
    x += mu * dt + p.vol * std::sqrt(dt) * s.Normal();
    const auto jumps =
        static_cast<double>(ch02::SamplePoisson(s, p.jump_intensity * dt));
    if (jumps > 0) {
      x += jumps * p.jump_mean + std::sqrt(jumps) * p.jump_vol * s.Normal();
    }
    out[i] = std::exp(x);
    prev_t = times[i];
  }
}

ch01::Estimate MertonCallMc(const MertonParams& p, double strike,
                            double maturity, std::size_t paths,
                            ch02::Sampler& s) {
  const double grid[1] = {maturity};
  double st[1];
  const double df = std::exp(-p.rate * maturity);
  ch01::RunningStats stats;
  for (std::size_t i = 0; i < paths; ++i) {
    MertonPath(p, grid, s, st);
    stats.Add(df * std::max(st[0] - strike, 0.0));
  }
  return stats.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch03
