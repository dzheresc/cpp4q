// Chapter 16 (continued): the matrix class at work.
//
// A multi-asset Black-Scholes model: each stock is lognormal with its own
// volatility, and the driving Brownian motions have correlation matrix
// rho. Correlated normals are L z with L the Cholesky factor of rho,
// computed once. Basket and exchange options are priced by Monte Carlo;
// Margrabe's formula prices the exchange option exactly.

#ifndef CPP4Q_ARMSTRONG_CH16_MATRIX_MULTI_STOCK_H_
#define CPP4Q_ARMSTRONG_CH16_MATRIX_MULTI_STOCK_H_

#include <span>
#include <vector>

#include "ch09_monte_carlo/monte_carlo.h"
#include "ch16_matrix/matrix.h"

namespace cpp4q::armstrong::ch16 {

using ch09::Estimate;
using ch09::Rng;

class MultiStockModel {
 public:
  // Throws unless sizes agree, spots and vols are positive and the
  // correlation matrix is a valid (SPD, unit-diagonal) correlation.
  MultiStockModel(std::vector<double> spots, std::vector<double> vols,
                  const Matrix& correlation, double rate);

  std::size_t n_stocks() const { return spots_.size(); }
  double rate() const { return rate_; }
  const std::vector<double>& spots() const { return spots_; }
  const std::vector<double>& vols() const { return vols_; }
  const Matrix& cholesky() const { return cholesky_; }

  // Risk-neutral S_i(T) for every stock, written into `out`.
  void SimulateTerminal(double maturity, Rng& rng,
                        std::span<double> out) const;

 private:
  std::vector<double> spots_;
  std::vector<double> vols_;
  Matrix cholesky_;
  double rate_;
};

// (sum_i w_i S_i(T) - K)^+ by Monte Carlo.
Estimate PriceBasketCall(const MultiStockModel& model,
                         std::span<const double> weights, double strike,
                         double maturity, int n_scenarios, Rng& rng);

// (S_0(T) - S_1(T))^+ on the first two stocks, by Monte Carlo.
Estimate PriceExchangeOption(const MultiStockModel& model, double maturity,
                             int n_scenarios, Rng& rng);

// Margrabe: S1 N(d1) - S2 N(d2), sigma^2 = s1^2 + s2^2 - 2 rho s1 s2.
double MargrabePrice(double s1, double s2, double vol1, double vol2,
                     double rho, double maturity);

}  // namespace cpp4q::armstrong::ch16

#endif  // CPP4Q_ARMSTRONG_CH16_MATRIX_MULTI_STOCK_H_
