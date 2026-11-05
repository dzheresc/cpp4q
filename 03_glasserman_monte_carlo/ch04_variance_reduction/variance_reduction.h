// Chapter 4: Variance reduction techniques.
//
// Each estimator prices a contract with a known answer so the variance
// reduction can be measured: antithetic variates (4.2), control variates
// (4.1: the underlying for a European call, the geometric Asian for the
// arithmetic Asian), stratified sampling (4.3: of the single normal for a
// European, and of W(T) via the Brownian bridge for an Asian), and
// importance sampling by a mean shift (4.6) for out-of-the-money calls.
//
// All return a CLT Estimate whose std_error is valid for that estimator
// (for antithetics a "sample" is one pair; for stratification the
// standard error comes from the within-stratum variances).

#ifndef CPP4Q_GLASSERMAN_CH04_VARIANCE_REDUCTION_VARIANCE_REDUCTION_H_
#define CPP4Q_GLASSERMAN_CH04_VARIANCE_REDUCTION_VARIANCE_REDUCTION_H_

#include <cstddef>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch04 {

using ch01::BlackScholesInputs;
using ch01::Estimate;

// Accumulates (Y, X) pairs and forms Y - beta (X - E[X]) with the sample-
// optimal beta = Cov(X, Y) / Var(X). Using the same sample for beta adds a
// bias of order 1/n (Glasserman 4.1.3), negligible at these sizes.
class ControlVariateAccumulator {
 public:
  void Add(double y, double x);
  std::size_t count() const { return n_; }
  double Beta() const;
  double Correlation() const;
  Estimate Result(double control_mean) const;

 private:
  std::size_t n_ = 0;
  double mean_y_ = 0.0;
  double mean_x_ = 0.0;
  double cyy_ = 0.0;
  double cxx_ = 0.0;
  double cxy_ = 0.0;
};

struct ControlVariateResult {
  Estimate estimate;
  double beta;
  double correlation;
};

// European call.
Estimate PlainCall(const BlackScholesInputs& in, std::size_t paths,
                   ch02::Sampler& s);
Estimate AntitheticCall(const BlackScholesInputs& in, std::size_t pairs,
                        ch02::Sampler& s);
// Control: the discounted terminal price, E = S0 exp(-q T).
ControlVariateResult ControlVariateCall(const BlackScholesInputs& in,
                                        std::size_t paths, ch02::Sampler& s);
// Equiprobable strata of the driving normal, n per stratum (n >= 2).
Estimate StratifiedCall(const BlackScholesInputs& in, std::size_t strata,
                        std::size_t per_stratum, ch02::Sampler& s);
// Z ~ N(shift, 1) with likelihood ratio exp(-shift Z + shift^2 / 2).
Estimate ImportanceSamplingCall(const BlackScholesInputs& in, double shift,
                                std::size_t paths, ch02::Sampler& s);
// The shift that centres S(T) on the strike: -d2.
double StrikeCenteringShift(const BlackScholesInputs& in);

// Arithmetic Asian call with `fixings` equally spaced fixings.
struct AsianSpec {
  BlackScholesInputs market;
  int fixings = 12;
};
Estimate PlainAsian(const AsianSpec& spec, std::size_t paths, ch02::Sampler& s);
Estimate AntitheticAsian(const AsianSpec& spec, std::size_t pairs,
                         ch02::Sampler& s);
// Kemna-Vorst: the geometric Asian (closed form) as control.
ControlVariateResult GeometricControlAsian(const AsianSpec& spec,
                                           std::size_t paths, ch02::Sampler& s);
// Stratify W(T) and fill the path with the Brownian bridge.
Estimate TerminalStratifiedAsian(const AsianSpec& spec, std::size_t strata,
                                 std::size_t per_stratum, ch02::Sampler& s);

}  // namespace cpp4q::glasserman::ch04

#endif  // CPP4Q_GLASSERMAN_CH04_VARIANCE_REDUCTION_VARIANCE_REDUCTION_H_
