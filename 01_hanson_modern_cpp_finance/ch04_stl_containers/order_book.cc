#include "ch04_stl_containers/order_book.h"

#include <algorithm>
#include <iterator>

namespace cpp4q::hanson::ch04 {
namespace {

template <typename Map>
void ApplyUpdate(Map& levels, double price, std::int64_t quantity) {
  // try_emplace + iterator avoids a second lookup.
  auto [it, inserted] = levels.try_emplace(price, 0);
  it->second += quantity;
  if (it->second <= 0) levels.erase(it);
}

template <typename Map>
std::optional<Level> Top(const Map& levels) {
  if (levels.empty()) return std::nullopt;
  const auto& [price, qty] = *levels.begin();
  return Level{price, qty};
}

template <typename Map>
std::vector<Level> TopN(const Map& levels, std::size_t depth) {
  std::vector<Level> out;
  out.reserve(std::min(depth, levels.size()));
  for (auto it = levels.begin(); it != levels.end() && out.size() < depth;
       ++it) {
    out.push_back({it->first, it->second});
  }
  return out;
}

template <typename Map>
std::vector<Fill> Consume(Map& levels, std::int64_t quantity) {
  std::vector<Fill> fills;
  auto it = levels.begin();
  while (quantity > 0 && it != levels.end()) {
    const std::int64_t take = std::min(quantity, it->second);
    fills.push_back({it->first, take});
    quantity -= take;
    it->second -= take;
    it = it->second == 0 ? levels.erase(it) : std::next(it);
  }
  return fills;
}

}  // namespace

void OrderBook::Update(Side side, double price, std::int64_t quantity) {
  if (side == Side::kBid) {
    ApplyUpdate(bids_, price, quantity);
  } else {
    ApplyUpdate(asks_, price, quantity);
  }
}

std::optional<Level> OrderBook::BestBid() const { return Top(bids_); }
std::optional<Level> OrderBook::BestAsk() const { return Top(asks_); }

std::optional<double> OrderBook::Mid() const {
  const auto bid = BestBid();
  const auto ask = BestAsk();
  if (!bid || !ask) return std::nullopt;
  return 0.5 * (bid->price + ask->price);
}

std::vector<Level> OrderBook::Depth(Side side, std::size_t depth) const {
  return side == Side::kBid ? TopN(bids_, depth) : TopN(asks_, depth);
}

std::vector<Fill> OrderBook::ExecuteMarket(Side aggressor,
                                           std::int64_t quantity) {
  // A buyer lifts offers, a seller hits bids.
  return aggressor == Side::kBid ? Consume(asks_, quantity)
                                 : Consume(bids_, quantity);
}

std::size_t OrderBook::level_count(Side side) const {
  return side == Side::kBid ? bids_.size() : asks_.size();
}

}  // namespace cpp4q::hanson::ch04
