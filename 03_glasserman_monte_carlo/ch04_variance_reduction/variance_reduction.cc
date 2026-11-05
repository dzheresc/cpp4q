#include "ch04_variance_reduction/variance_reduction.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/gbm.h"

namespace cpp4q::glasserman::ch04 {
namespace {

// S(T) = spot * exp(drift + sd * z), discounted call payoff.
struct TerminalCall {
  explicit TerminalCall(const BlackScholesInputs& in)
      : spot(in.spot),
        strike(in.strike),
        drift((in.rate - in.dividend - 0.5 * in.vol * in.vol) * in.maturity),
        sd(in.vol * std::sqrt(in.maturity)),
        df(std::exp(-in.rate * in.maturity)) {}

  double Terminal(double z) const { return spot * std::exp(drift + sd * z); }
  double Payoff(double z) const {
    return df * std::max(Terminal(z) - strike, 0.0);
  }

  double spot, strike, drift, sd, df;
};

// Discounted arithmetic and geometric Asian payoffs of one path.
struct AsianPayoffs {
  double arithmetic;
  double geometric;
};

class AsianPath {
 public:
  explicit AsianPath(const AsianSpec& spec)
      : spec_(spec),
        times_(ch03::UniformGrid(spec.market.maturity,
                                 static_cast<std::size_t>(spec.fixings))),
        gbm_{spec.market.spot, spec.market.rate - spec.market.dividend,
             spec.market.vol},
        df_(std::exp(-spec.market.rate * spec.market.maturity)),
        z_(times_.size()) {}

  std::span<double> normals() { return z_; }

  AsianPayoffs FromNormals() {
    // The log path is built anyway, so the geometric mean comes from it
    // directly instead of from n calls to log() (callgrind: log() was 20%
    // of the chapter 4 workload's instructions).
    const double mu = gbm_.drift - 0.5 * gbm_.vol * gbm_.vol;
    double prev_t = 0.0;
    double x = std::log(gbm_.spot);
    double sum = 0.0;
    double log_sum = 0.0;
    for (std::size_t i = 0; i < times_.size(); ++i) {
      const double dt = times_[i] - prev_t;
      x += mu * dt + gbm_.vol * std::sqrt(dt) * z_[i];
      sum += std::exp(x);
      log_sum += x;
      prev_t = times_[i];
    }
    return Payoffs(sum, log_sum);
  }

  AsianPayoffs FromBrownian(std::span<const double> w) {
    const double mu = gbm_.drift - 0.5 * gbm_.vol * gbm_.vol;
    const double log_s0 = std::log(gbm_.spot);
    double sum = 0.0;
    double log_sum = 0.0;
    for (std::size_t i = 0; i < times_.size(); ++i) {
      const double x = log_s0 + mu * times_[i] + gbm_.vol * w[i];
      sum += std::exp(x);
      log_sum += x;
    }
    return Payoffs(sum, log_sum);
  }

  const std::vector<double>& times() const { return times_; }

 private:
  AsianPayoffs Payoffs(double sum, double log_sum) const {
    const auto n = static_cast<double>(times_.size());
    const double k = spec_.market.strike;
    return {df_ * std::max(sum / n - k, 0.0),
            df_ * std::max(std::exp(log_sum / n) - k, 0.0)};
  }

  AsianSpec spec_;
  std::vector<double> times_;
  ch03::GbmParams gbm_;
  double df_;
  std::vector<double> z_;
};

void CheckStrata(std::size_t strata, std::size_t per_stratum) {
  if (strata == 0 || per_stratum < 2) {
    throw std::invalid_argument("need strata >= 1 and >= 2 draws per stratum");
  }
}

// Combines per-stratum statistics of equiprobable strata.
class StratifiedAccumulator {
 public:
  explicit StratifiedAccumulator(std::size_t strata) : strata_(strata) {}
  void AddStratum(const ch01::RunningStats& st) {
    mean_ += st.Mean();
    var_ += st.Variance() / static_cast<double>(st.count());
    samples_ += st.count();
  }
  Estimate Result() const {
    const auto k = static_cast<double>(strata_);
    return {mean_ / k, std::sqrt(var_) / k, samples_};
  }

 private:
  std::size_t strata_;
  double mean_ = 0.0;
  double var_ = 0.0;
  std::size_t samples_ = 0;
};

}  // namespace

void ControlVariateAccumulator::Add(double y, double x) {
  ++n_;
  const double inv_n = 1.0 / static_cast<double>(n_);
  const double dx = x - mean_x_;
  const double dy = y - mean_y_;
  mean_x_ += dx * inv_n;
  mean_y_ += dy * inv_n;
  cxx_ += dx * (x - mean_x_);
  cyy_ += dy * (y - mean_y_);
  cxy_ += dx * (y - mean_y_);
}

double ControlVariateAccumulator::Beta() const {
  return cxx_ > 0.0 ? cxy_ / cxx_ : 0.0;
}

double ControlVariateAccumulator::Correlation() const {
  return (cxx_ > 0.0 && cyy_ > 0.0) ? cxy_ / std::sqrt(cxx_ * cyy_) : 0.0;
}

Estimate ControlVariateAccumulator::Result(double control_mean) const {
  if (n_ < 3) throw std::logic_error("need at least 3 samples");
  const double beta = Beta();
  const double mean = mean_y_ - beta * (mean_x_ - control_mean);
  // Residual variance of the regression of Y on X (n - 2 dof).
  const double resid =
      std::max(cyy_ - beta * cxy_, 0.0) / static_cast<double>(n_ - 2);
  return {mean, std::sqrt(resid / static_cast<double>(n_)), n_};
}

Estimate PlainCall(const BlackScholesInputs& in, std::size_t paths,
                   ch02::Sampler& s) {
  const TerminalCall call(in);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) st.Add(call.Payoff(s.Normal()));
  return st.ToEstimate();
}

Estimate AntitheticCall(const BlackScholesInputs& in, std::size_t pairs,
                        ch02::Sampler& s) {
  const TerminalCall call(in);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < pairs; ++i) {
    const double z = s.Normal();
    st.Add(0.5 * (call.Payoff(z) + call.Payoff(-z)));
  }
  return st.ToEstimate();
}

ControlVariateResult ControlVariateCall(const BlackScholesInputs& in,
                                        std::size_t paths, ch02::Sampler& s) {
  const TerminalCall call(in);
  ControlVariateAccumulator acc;
  for (std::size_t i = 0; i < paths; ++i) {
    const double st = call.Terminal(s.Normal());
    acc.Add(call.df * std::max(st - in.strike, 0.0), call.df * st);
  }
  const double control_mean = in.spot * std::exp(-in.dividend * in.maturity);
  return {acc.Result(control_mean), acc.Beta(), acc.Correlation()};
}

Estimate StratifiedCall(const BlackScholesInputs& in, std::size_t strata,
                        std::size_t per_stratum, ch02::Sampler& s) {
  CheckStrata(strata, per_stratum);
  const TerminalCall call(in);
  const auto k = static_cast<double>(strata);
  StratifiedAccumulator acc(strata);
  for (std::size_t j = 0; j < strata; ++j) {
    ch01::RunningStats st;
    for (std::size_t i = 0; i < per_stratum; ++i) {
      const double u = (static_cast<double>(j) + s.Uniform()) / k;
      st.Add(call.Payoff(ch02::InverseNormalCdf(u)));
    }
    acc.AddStratum(st);
  }
  return acc.Result();
}

Estimate ImportanceSamplingCall(const BlackScholesInputs& in, double shift,
                                std::size_t paths, ch02::Sampler& s) {
  const TerminalCall call(in);
  const double half_sq = 0.5 * shift * shift;
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    const double z = s.Normal() + shift;
    st.Add(call.Payoff(z) * std::exp(-shift * z + half_sq));
  }
  return st.ToEstimate();
}

double StrikeCenteringShift(const BlackScholesInputs& in) {
  return (std::log(in.strike / in.spot) -
          (in.rate - in.dividend - 0.5 * in.vol * in.vol) * in.maturity) /
         (in.vol * std::sqrt(in.maturity));
}

Estimate PlainAsian(const AsianSpec& spec, std::size_t paths,
                    ch02::Sampler& s) {
  AsianPath path(spec);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    s.FillNormal(path.normals());
    st.Add(path.FromNormals().arithmetic);
  }
  return st.ToEstimate();
}

Estimate AntitheticAsian(const AsianSpec& spec, std::size_t pairs,
                         ch02::Sampler& s) {
  AsianPath path(spec);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < pairs; ++i) {
    s.FillNormal(path.normals());
    const double a = path.FromNormals().arithmetic;
    for (double& z : path.normals()) z = -z;
    const double b = path.FromNormals().arithmetic;
    st.Add(0.5 * (a + b));
  }
  return st.ToEstimate();
}

ControlVariateResult GeometricControlAsian(const AsianSpec& spec,
                                           std::size_t paths,
                                           ch02::Sampler& s) {
  AsianPath path(spec);
  ControlVariateAccumulator acc;
  for (std::size_t i = 0; i < paths; ++i) {
    s.FillNormal(path.normals());
    const AsianPayoffs p = path.FromNormals();
    acc.Add(p.arithmetic, p.geometric);
  }
  const double control_mean =
      ch01::GeometricAsianCallPrice(spec.market, spec.fixings);
  return {acc.Result(control_mean), acc.Beta(), acc.Correlation()};
}

Estimate TerminalStratifiedAsian(const AsianSpec& spec, std::size_t strata,
                                 std::size_t per_stratum, ch02::Sampler& s) {
  CheckStrata(strata, per_stratum);
  AsianPath path(spec);
  const ch03::BrownianBridge bridge(path.times());
  std::vector<double> w(path.times().size());
  const auto k = static_cast<double>(strata);
  StratifiedAccumulator acc(strata);
  for (std::size_t j = 0; j < strata; ++j) {
    ch01::RunningStats st;
    for (std::size_t i = 0; i < per_stratum; ++i) {
      auto z = path.normals();
      s.FillNormal(z);
      z[0] = ch02::InverseNormalCdf((static_cast<double>(j) + s.Uniform()) / k);
      bridge.Build(z, w);
      st.Add(path.FromBrownian(w).arithmetic);
    }
    acc.AddStratum(st);
  }
  return acc.Result();
}

}  // namespace cpp4q::glasserman::ch04
