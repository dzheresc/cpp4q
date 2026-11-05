#include "ch08_american_options/lsm.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch08 {

void SolveNormalEquations(std::vector<double>& a, std::vector<double>& b,
                          std::size_t k) {
  double trace = 0.0;
  for (std::size_t i = 0; i < k; ++i) trace += a[i * k + i];
  const double ridge = 1e-12 * (trace > 0.0 ? trace : 1.0);
  // In-place Cholesky: lower triangle of a becomes L.
  for (std::size_t j = 0; j < k; ++j) {
    double diag = a[j * k + j];
    for (std::size_t m = 0; m < j; ++m) diag -= a[j * k + m] * a[j * k + m];
    if (diag <= ridge) diag += ridge;
    const double ljj = std::sqrt(diag);
    a[j * k + j] = ljj;
    for (std::size_t i = j + 1; i < k; ++i) {
      double v = a[i * k + j];
      for (std::size_t m = 0; m < j; ++m) v -= a[i * k + m] * a[j * k + m];
      a[i * k + j] = v / ljj;
    }
  }
  for (std::size_t i = 0; i < k; ++i) {  // L y = b
    double v = b[i];
    for (std::size_t m = 0; m < i; ++m) v -= a[i * k + m] * b[m];
    b[i] = v / a[i * k + i];
  }
  for (std::size_t i = k; i-- > 0;) {  // L^T x = y
    double v = b[i];
    for (std::size_t m = i + 1; m < k; ++m) v -= a[m * k + i] * b[m];
    b[i] = v / a[i * k + i];
  }
}

LsmPricer::LsmPricer(const BermudanSpec& spec, std::size_t basis_size)
    : spec_(spec), basis_size_(basis_size) {
  if (spec.exercise_dates == 0) throw std::invalid_argument("no dates");
  if (basis_size == 0 || basis_size > 8) {
    throw std::invalid_argument("basis size must be in [1, 8]");
  }
  dt_ = spec.market.maturity / static_cast<double>(spec.exercise_dates);
  step_disc_ = std::exp(-spec.market.rate * dt_);
}

double LsmPricer::Payoff(double s) const {
  const double k = spec_.market.strike;
  return spec_.type == ch01::OptionType::kPut ? std::max(k - s, 0.0)
                                              : std::max(s - k, 0.0);
}

double LsmPricer::Continuation(std::size_t date, double s) const {
  const auto& beta = coefficients_[date];
  const double x = s / spec_.market.strike;
  double value = 0.0;
  for (std::size_t j = basis_size_; j-- > 0;) value = value * x + beta[j];
  return value;
}

void LsmPricer::SimulatePaths(std::size_t paths, std::uint64_t seed,
                              std::vector<double>& spots) const {
  // Date-major: row `date` holds S(t_date) for every path. The backward
  // sweep reads one date across all paths, so this layout streams through
  // memory; the first, path-major version strode exercise_dates * 8 bytes
  // between consecutive reads (see profiling/REPORT.md).
  const std::size_t m = spec_.exercise_dates;
  const auto& mk = spec_.market;
  const double drift = (mk.rate - mk.dividend - 0.5 * mk.vol * mk.vol) * dt_;
  const double sd = mk.vol * std::sqrt(dt_);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  spots.resize(paths * m);
  for (std::size_t date = 0; date < m; ++date) {
    double* row = spots.data() + date * paths;
    const double* prev = date == 0 ? nullptr : row - paths;
    for (std::size_t i = 0; i < paths; ++i) {
      const double from = prev ? prev[i] : mk.spot;
      row[i] = from * std::exp(drift + sd * s.Normal());
    }
  }
}

ch01::Estimate LsmPricer::Train(std::size_t paths, std::uint64_t seed) {
  const std::size_t m = spec_.exercise_dates;
  const std::size_t k = basis_size_;
  std::vector<double> spots;
  SimulatePaths(paths, seed, spots);
  // cash[i]: value of path i's future cash flow, discounted to the
  // current date of the backward sweep.
  std::vector<double> cash(paths);
  const double* last = spots.data() + (m - 1) * paths;
  for (std::size_t i = 0; i < paths; ++i) cash[i] = Payoff(last[i]);
  coefficients_.assign(m - 1, std::vector<double>(k, 0.0));
  fitted_.assign(m - 1, false);
  std::vector<double> ata(k * k), atb(k), basis(k);
  for (std::size_t date = m - 1; date-- > 0;) {
    for (double& c : cash) c *= step_disc_;
    std::fill(ata.begin(), ata.end(), 0.0);
    std::fill(atb.begin(), atb.end(), 0.0);
    std::size_t itm = 0;
    const double* row = spots.data() + date * paths;
    for (std::size_t i = 0; i < paths; ++i) {
      const double s = row[i];
      if (Payoff(s) <= 0.0) continue;
      ++itm;
      const double x = s / spec_.market.strike;
      basis[0] = 1.0;
      for (std::size_t j = 1; j < k; ++j) basis[j] = basis[j - 1] * x;
      for (std::size_t r = 0; r < k; ++r) {
        for (std::size_t c = 0; c < k; ++c)
          ata[r * k + c] += basis[r] * basis[c];
        atb[r] += basis[r] * cash[i];
      }
    }
    if (itm < k) continue;  // too few points: never exercise here
    SolveNormalEquations(ata, atb, k);
    coefficients_[date] = atb;
    fitted_[date] = true;
    for (std::size_t i = 0; i < paths; ++i) {
      const double s = row[i];
      const double h = Payoff(s);
      if (h > 0.0 && h >= Continuation(date, s)) cash[i] = h;
    }
  }
  ch01::RunningStats st;
  for (double c : cash) st.Add(c * step_disc_);
  return st.ToEstimate();
}

ch01::Estimate LsmPricer::Price(std::size_t paths, std::uint64_t seed) const {
  if (fitted_.size() + 1 != spec_.exercise_dates) {
    throw std::logic_error("Train() first");
  }
  // Paths are simulated one at a time and stopped at exercise: no path
  // matrix (the first version stored paths x dates doubles) and no normals
  // drawn past the stopping date.
  const std::size_t m = spec_.exercise_dates;
  const auto& mk = spec_.market;
  const double drift = (mk.rate - mk.dividend - 0.5 * mk.vol * mk.vol) * dt_;
  const double sd = mk.vol * std::sqrt(dt_);
  ch02::Sampler sampler{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    double value = 0.0;
    double disc = 1.0;
    double s = mk.spot;
    for (std::size_t date = 0; date < m; ++date) {
      disc *= step_disc_;
      s *= std::exp(drift + sd * sampler.Normal());
      const double h = Payoff(s);
      if (h <= 0.0) continue;
      const bool last = date + 1 == m;
      if (last || (fitted_[date] && h >= Continuation(date, s))) {
        value = disc * h;
        break;
      }
    }
    st.Add(value);
  }
  return st.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch08
