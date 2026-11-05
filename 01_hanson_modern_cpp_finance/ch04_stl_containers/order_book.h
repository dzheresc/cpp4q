// Chapter 4: The STL Part I - Containers and Iterators.
//
// A price-level limit order book. Ordered std::map keyed by price gives the
// best bid/ask at begin(); std::greater<> flips the bid side so both sides
// iterate best-first.

#ifndef CPP4Q_HANSON_CH04_STL_CONTAINERS_ORDER_BOOK_H_
#define CPP4Q_HANSON_CH04_STL_CONTAINERS_ORDER_BOOK_H_

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace cpp4q::hanson::ch04 {

enum class Side { kBid, kAsk };

struct Level {
  double price;
  std::int64_t quantity;
  friend bool operator==(const Level&, const Level&) = default;
};

struct Fill {
  double price;
  std::int64_t quantity;
};

class OrderBook {
 public:
  // Adds (or with negative quantity removes) resting size at a price. Levels
  // that drop to zero are erased.
  void Update(Side side, double price, std::int64_t quantity);

  [[nodiscard]] std::optional<Level> BestBid() const;
  [[nodiscard]] std::optional<Level> BestAsk() const;
  [[nodiscard]] std::optional<double> Mid() const;

  // Top `depth` levels, best first.
  [[nodiscard]] std::vector<Level> Depth(Side side, std::size_t depth) const;

  // Walks the opposite side consuming liquidity; returns the fills.
  std::vector<Fill> ExecuteMarket(Side aggressor, std::int64_t quantity);

  [[nodiscard]] std::size_t level_count(Side side) const;

 private:
  std::map<double, std::int64_t, std::greater<>> bids_;
  std::map<double, std::int64_t, std::less<>> asks_;
};

}  // namespace cpp4q::hanson::ch04

#endif  // CPP4Q_HANSON_CH04_STL_CONTAINERS_ORDER_BOOK_H_
