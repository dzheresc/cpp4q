#include "ch13_portfolio/portfolio.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cpp4q::armstrong::ch13 {

double ZeroCouponBond::Price(const BlackScholesModel& m) const {
  const double tau = maturity_ - m.date;
  if (tau < 0) throw std::invalid_argument("bond has matured");
  return notional_ * std::exp(-m.risk_free_rate * tau);
}

std::size_t Portfolio::Add(double quantity,
                           std::shared_ptr<const Priceable> security) {
  if (!security) throw std::invalid_argument("null security");
  positions_.push_back({quantity, std::move(security)});
  return positions_.size() - 1;
}

void Portfolio::SetQuantity(std::size_t index, double quantity) {
  positions_.at(index).quantity = quantity;
}

double Portfolio::quantity(std::size_t index) const {
  return positions_.at(index).quantity;
}

double Portfolio::Price(const BlackScholesModel& m) const {
  double total = 0.0;
  for (const Position& p : positions_) {
    total += p.quantity * p.security->Price(m);
  }
  return total;
}

std::vector<double> Portfolio::PositionValues(
    const BlackScholesModel& m) const {
  std::vector<double> values;
  values.reserve(positions_.size());
  for (const Position& p : positions_) {
    values.push_back(p.quantity * p.security->Price(m));
  }
  return values;
}

double Portfolio::Delta(const BlackScholesModel& m, double rel_bump) const {
  const double h = rel_bump * m.stock_price;
  BlackScholesModel up = m, down = m;
  up.stock_price += h;
  down.stock_price -= h;
  return (Price(up) - Price(down)) / (2 * h);
}

double Portfolio::Gamma(const BlackScholesModel& m, double rel_bump) const {
  const double h = rel_bump * m.stock_price;
  BlackScholesModel up = m, down = m;
  up.stock_price += h;
  down.stock_price -= h;
  return (Price(up) - 2 * Price(m) + Price(down)) / (h * h);
}

double Portfolio::Vega(const BlackScholesModel& m, double vol_bump) const {
  BlackScholesModel up = m, down = m;
  up.volatility += vol_bump;
  down.volatility -= vol_bump;
  return (Price(up) - Price(down)) / (2 * vol_bump);
}

Portfolio PutCallParityPortfolio(double strike, double maturity) {
  Portfolio p;
  p.Add(1, std::make_shared<ch10::EuropeanCall>(strike, maturity));
  p.Add(-1, std::make_shared<ch10::EuropeanPut>(strike, maturity));
  p.Add(-1, std::make_shared<Stock>());
  p.Add(strike, std::make_shared<ZeroCouponBond>(1.0, maturity));
  return p;
}

}  // namespace cpp4q::armstrong::ch13
