#include "ch05_quasi_monte_carlo/qmc_pricing.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch02_random_numbers/normal.h"
#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/gbm.h"
#include "ch05_quasi_monte_carlo/low_discrepancy.h"

namespace cpp4q::glasserman::ch05 {
namespace {

class AsianEvaluator {
 public:
  explicit AsianEvaluator(const QmcAsianSpec& spec)
      : spec_(spec),
        times_(ch03::UniformGrid(spec.market.maturity,
                                 static_cast<std::size_t>(spec.fixings))),
        bridge_(times_),
        gbm_{spec.market.spot, spec.market.rate - spec.market.dividend,
             spec.market.vol},
        df_(std::exp(-spec.market.rate * spec.market.maturity)),
        w_(times_.size()),
        s_(times_.size()) {}

  std::size_t dim() const { return times_.size(); }

  // Discounted payoff from d standard normals.
  double Payoff(std::span<const double> z, PathConstruction c) {
    if (c == PathConstruction::kBrownianBridge) {
      bridge_.Build(z, w_);
    } else {
      ch03::RandomWalkPath(times_, z, w_);
    }
    ch03::GbmFromBrownian(gbm_, times_, w_, s_);
    double sum = 0.0;
    for (double x : s_) sum += x;
    return df_ *
           std::max(sum / static_cast<double>(s_.size()) - spec_.market.strike,
                    0.0);
  }

 private:
  QmcAsianSpec spec_;
  std::vector<double> times_;
  ch03::BrownianBridge bridge_;
  ch03::GbmParams gbm_;
  double df_;
  std::vector<double> w_;
  std::vector<double> s_;
};

}  // namespace

double QmcAsianCall(const QmcAsianSpec& spec, std::size_t points,
                    PathConstruction construction, std::uint64_t shift_seed) {
  if (spec.fixings < 1 || spec.fixings > static_cast<int>(Sobol::kMaxDim)) {
    throw std::invalid_argument("fixings must be in [1, 16]");
  }
  AsianEvaluator eval(spec);
  Sobol sobol = shift_seed ? Sobol(eval.dim(), shift_seed) : Sobol(eval.dim());
  std::vector<double> u(eval.dim());
  double sum = 0.0;
  for (std::size_t i = 0; i < points; ++i) {
    sobol.Next(u);
    for (double& x : u) x = ch02::InverseNormalCdf(x);
    sum += eval.Payoff(u, construction);
  }
  return sum / static_cast<double>(points);
}

ch01::Estimate RqmcAsianCall(const QmcAsianSpec& spec, std::size_t points,
                             std::size_t replications,
                             PathConstruction construction,
                             std::uint64_t seed) {
  ch01::RunningStats st;
  for (std::size_t r = 0; r < replications; ++r) {
    st.Add(QmcAsianCall(spec, points, construction, seed * 1000003 + r + 1));
  }
  ch01::Estimate e = st.ToEstimate();
  e.samples = points * replications;
  return e;
}

ch01::Estimate McAsianCall(const QmcAsianSpec& spec, std::size_t paths,
                           std::uint64_t seed) {
  AsianEvaluator eval(spec);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  std::vector<double> z(eval.dim());
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    s.FillNormal(z);
    st.Add(eval.Payoff(z, PathConstruction::kRandomWalk));
  }
  return st.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch05
