// Chapter 4: associative containers for positions and prices.

#ifndef CPP4Q_HANSON_CH04_STL_CONTAINERS_PORTFOLIO_H_
#define CPP4Q_HANSON_CH04_STL_CONTAINERS_PORTFOLIO_H_

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cpp4q::hanson::ch04 {

struct Position {
  double quantity = 0.0;
  double average_cost = 0.0;
};

// Transparent hash so unordered_map<std::string, ...> can be queried with a
// std::string_view without building a temporary std::string.
struct StringHash {
  using is_transparent = void;
  std::size_t operator()(std::string_view s) const {
    return std::hash<std::string_view>{}(s);
  }
};

using PriceMap =
    std::unordered_map<std::string, double, StringHash, std::equal_to<>>;

class Portfolio {
 public:
  // Signed quantity: positive buys, negative sells. Average cost tracks the
  // open position; it resets when the position flips or closes.
  void Trade(std::string_view ticker, double quantity, double price);

  [[nodiscard]] double MarketValue(const PriceMap& prices) const;
  [[nodiscard]] double UnrealizedPnl(const PriceMap& prices) const;
  [[nodiscard]] double realized_pnl() const { return realized_pnl_; }

  // nullptr if absent.
  [[nodiscard]] const Position* Find(std::string_view ticker) const;
  [[nodiscard]] std::vector<std::string> Tickers() const;  // sorted
  [[nodiscard]] std::size_t size() const { return positions_.size(); }

 private:
  // std::map keeps tickers sorted for reporting; std::less<> enables
  // heterogeneous lookup with string_view.
  std::map<std::string, Position, std::less<>> positions_;
  double realized_pnl_ = 0.0;
};

}  // namespace cpp4q::hanson::ch04

#endif  // CPP4Q_HANSON_CH04_STL_CONTAINERS_PORTFOLIO_H_
