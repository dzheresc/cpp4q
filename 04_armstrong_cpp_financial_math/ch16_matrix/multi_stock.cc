#include "ch16_matrix/multi_stock.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch16 {
namespace {

// Mean and standard error of discounted payoffs.
class Accumulator {
 public:
  void Add(double x) {
    ++n_;
    const double d = x - mean_;
    mean_ += d / static_cast<double>(n_);
    m2_ += d * (x - mean_);
  }
  Estimate Result(double discount) const {
    const double n = static_cast<double>(n_);
    return {discount * mean_, discount * std::sqrt(m2_ / (n - 1) / n)};
  }

 private:
  std::size_t n_ = 0;
  double mean_ = 0.0;
  double m2_ = 0.0;
};

}  // namespace

MultiStockModel::MultiStockModel(std::vector<double> spots,
                                 std::vector<double> vols,
                                 const Matrix& correlation, double rate)
    : spots_(std::move(spots)), vols_(std::move(vols)), rate_(rate) {
  const std::size_t n = spots_.size();
  if (n == 0 || vols_.size() != n || correlation.rows() != n ||
      correlation.cols() != n) {
    throw std::invalid_argument("inconsistent model sizes");
  }
  for (std::size_t i = 0; i < n; ++i) {
    if (!(spots_[i] > 0 && vols_[i] > 0)) {
      throw std::invalid_argument("spots and vols must be positive");
    }
    if (std::abs(correlation(i, i) - 1.0) > 1e-12) {
      throw std::invalid_argument("correlation diagonal must be 1");
    }
  }
  cholesky_ = Cholesky(correlation);
}

void MultiStockModel::SimulateTerminal(double maturity, Rng& rng,
                                       std::span<double> out) const {
  std::normal_distribution<double> normal;
  std::vector<double> z(n_stocks());
  SimulateTerminal(maturity, rng, normal, z, out);
}

void MultiStockModel::SimulateTerminal(double maturity, Rng& rng,
                                       std::normal_distribution<double>& normal,
                                       std::span<double> scratch,
                                       std::span<double> out) const {
  const std::size_t n = n_stocks();
  if (out.size() != n || scratch.size() != n) {
    throw std::invalid_argument("output size mismatch");
  }
  for (double& x : scratch) x = normal(rng);
  const double sqrt_t = std::sqrt(maturity);
  for (std::size_t i = 0; i < n; ++i) {
    // Row i of the lower-triangular factor times z: only k <= i.
    double w = 0.0;
    for (std::size_t k = 0; k <= i; ++k) w += cholesky_(i, k) * scratch[k];
    const double v = vols_[i];
    out[i] = spots_[i] *
             std::exp((rate_ - 0.5 * v * v) * maturity + v * sqrt_t * w);
  }
}

Estimate PriceBasketCall(const MultiStockModel& model,
                         std::span<const double> weights, double strike,
                         double maturity, int n_scenarios, Rng& rng) {
  if (weights.size() != model.n_stocks()) {
    throw std::invalid_argument("one weight per stock");
  }
  if (n_scenarios < 2) throw std::invalid_argument("need >= 2 scenarios");
  std::vector<double> s(model.n_stocks()), z(model.n_stocks());
  std::normal_distribution<double> normal;
  Accumulator acc;
  for (int k = 0; k < n_scenarios; ++k) {
    model.SimulateTerminal(maturity, rng, normal, z, s);
    double basket = 0.0;
    for (std::size_t i = 0; i < s.size(); ++i) basket += weights[i] * s[i];
    acc.Add(std::max(basket - strike, 0.0));
  }
  return acc.Result(std::exp(-model.rate() * maturity));
}

Estimate PriceExchangeOption(const MultiStockModel& model, double maturity,
                             int n_scenarios, Rng& rng) {
  if (model.n_stocks() < 2) throw std::invalid_argument("need two stocks");
  if (n_scenarios < 2) throw std::invalid_argument("need >= 2 scenarios");
  std::vector<double> s(model.n_stocks()), z(model.n_stocks());
  std::normal_distribution<double> normal;
  Accumulator acc;
  for (int k = 0; k < n_scenarios; ++k) {
    model.SimulateTerminal(maturity, rng, normal, z, s);
    acc.Add(std::max(s[0] - s[1], 0.0));
  }
  return acc.Result(std::exp(-model.rate() * maturity));
}

double MargrabePrice(double s1, double s2, double vol1, double vol2,
                     double rho, double maturity) {
  const double sigma =
      std::sqrt(vol1 * vol1 + vol2 * vol2 - 2 * rho * vol1 * vol2);
  const double sd = sigma * std::sqrt(maturity);
  const double d1 = (std::log(s1 / s2) + 0.5 * sd * sd) / sd;
  return s1 * ch03::NormCdf(d1) - s2 * ch03::NormCdf(d1 - sd);
}

}  // namespace cpp4q::armstrong::ch16
