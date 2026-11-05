// Chapter 3: Generating sample paths.
//
// Merton's jump-diffusion (3.5.1): GBM times a compound Poisson process of
// lognormal jumps, compensated so the discounted price is a martingale.
// On a fixed grid the number of jumps per interval is Poisson and, given
// N jumps, their total log size is normal, so the path is exact.

#ifndef CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_JUMP_DIFFUSION_H_
#define CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_JUMP_DIFFUSION_H_

#include <cstddef>
#include <span>

#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch03 {

struct MertonParams {
  double spot = 100.0;
  double rate = 0.05;
  double dividend = 0.0;
  double vol = 0.2;
  double jump_intensity = 1.0;  // jumps per year
  double jump_mean = -0.1;      // mean of log jump size
  double jump_vol = 0.15;       // sd of log jump size
};

// Series of Black-Scholes prices conditioned on the number of jumps.
double MertonCallPrice(const MertonParams& p, double strike, double maturity,
                       int terms = 80);

// s[i] = S(t_i) on the grid `times`.
void MertonPath(const MertonParams& p, std::span<const double> times,
                ch02::Sampler& s, std::span<double> out);

ch01::Estimate MertonCallMc(const MertonParams& p, double strike,
                            double maturity, std::size_t paths,
                            ch02::Sampler& s);

}  // namespace cpp4q::glasserman::ch03

#endif  // CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_JUMP_DIFFUSION_H_
