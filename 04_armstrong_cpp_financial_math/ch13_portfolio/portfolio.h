// Chapter 13: The Portfolio Class.
//
// A portfolio is a list of (quantity, security) positions where each
// security is any Priceable. Positions share ownership of their
// securities (std::shared_ptr<const Priceable>), so one contract can sit
// in several portfolios. The portfolio prices itself and computes its
// Greeks by bump-and-reprice with central differences, which works for
// any security without it having to know its own derivatives.

#ifndef CPP4Q_ARMSTRONG_CH13_PORTFOLIO_PORTFOLIO_H_
#define CPP4Q_ARMSTRONG_CH13_PORTFOLIO_PORTFOLIO_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "ch10_interfaces/interfaces.h"

namespace cpp4q::armstrong::ch13 {

using ch10::BlackScholesModel;
using ch10::Priceable;

// One share of the stock.
class Stock final : public Priceable {
 public:
  double Price(const BlackScholesModel& m) const override {
    return m.stock_price;
  }
};

// Pays `notional` at `maturity`.
class ZeroCouponBond final : public Priceable {
 public:
  ZeroCouponBond(double notional, double maturity)
      : notional_(notional), maturity_(maturity) {}
  double Price(const BlackScholesModel& m) const override;

 private:
  double notional_;
  double maturity_;
};

class Portfolio {
 public:
  // Returns the index of the new position.
  std::size_t Add(double quantity, std::shared_ptr<const Priceable> security);
  void SetQuantity(std::size_t index, double quantity);
  double quantity(std::size_t index) const;
  std::size_t size() const { return positions_.size(); }

  double Price(const BlackScholesModel& m) const;
  // Value of each position (quantity times price).
  std::vector<double> PositionValues(const BlackScholesModel& m) const;

  // Central differences: spot bumped by +-rel_bump * S, vol by +-vol_bump.
  double Delta(const BlackScholesModel& m, double rel_bump = 1e-4) const;
  double Gamma(const BlackScholesModel& m, double rel_bump = 1e-3) const;
  double Vega(const BlackScholesModel& m, double vol_bump = 1e-4) const;

 private:
  struct Position {
    double quantity;
    std::shared_ptr<const Priceable> security;
  };
  std::vector<Position> positions_;
};

// Long call, short put, short one share, long K zero-coupon bonds: by
// put-call parity this portfolio is worth exactly zero.
Portfolio PutCallParityPortfolio(double strike, double maturity);

}  // namespace cpp4q::armstrong::ch13

#endif  // CPP4Q_ARMSTRONG_CH13_PORTFOLIO_PORTFOLIO_H_
