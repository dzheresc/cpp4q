#include "ch04_stl_containers/portfolio.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cpp4q::hanson::ch04 {

void Portfolio::Trade(std::string_view ticker, double quantity, double price) {
  if (quantity == 0.0) return;
  auto it = positions_.find(ticker);
  if (it == positions_.end()) {
    it = positions_.emplace(std::string(ticker), Position{}).first;
  }
  Position& pos = it->second;

  const bool same_direction =
      pos.quantity == 0.0 ||
      std::signbit(pos.quantity) == std::signbit(quantity);
  if (same_direction) {
    const double new_qty = pos.quantity + quantity;
    pos.average_cost =
        (pos.average_cost * pos.quantity + price * quantity) / new_qty;
    pos.quantity = new_qty;
  } else {
    // Closing (part of) the position realises P&L on the closed amount.
    const double closed = std::copysign(
        std::min(std::abs(quantity), std::abs(pos.quantity)), pos.quantity);
    realized_pnl_ += closed * (price - pos.average_cost);
    pos.quantity += quantity;
    if (std::signbit(pos.quantity) != std::signbit(closed) &&
        pos.quantity != 0.0) {
      pos.average_cost = price;  // flipped: the remainder opened at `price`
    }
  }
  if (pos.quantity == 0.0) positions_.erase(it);
}

double Portfolio::MarketValue(const PriceMap& prices) const {
  double total = 0.0;
  for (const auto& [ticker, pos] : positions_) {
    const auto it = prices.find(ticker);
    if (it == prices.end()) {
      throw std::out_of_range("Portfolio: no price for " + ticker);
    }
    total += pos.quantity * it->second;
  }
  return total;
}

double Portfolio::UnrealizedPnl(const PriceMap& prices) const {
  double pnl = 0.0;
  for (const auto& [ticker, pos] : positions_) {
    pnl += pos.quantity * (prices.at(ticker) - pos.average_cost);
  }
  return pnl;
}

const Position* Portfolio::Find(std::string_view ticker) const {
  const auto it = positions_.find(ticker);
  return it == positions_.end() ? nullptr : &it->second;
}

std::vector<std::string> Portfolio::Tickers() const {
  std::vector<std::string> out;
  out.reserve(positions_.size());
  for (const auto& entry : positions_) out.push_back(entry.first);
  return out;
}

}  // namespace cpp4q::hanson::ch04
