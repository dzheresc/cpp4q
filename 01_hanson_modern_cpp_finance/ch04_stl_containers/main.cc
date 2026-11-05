// Chapter 4 demo: order book, portfolio, rolling window.

#include <iostream>
#include <numeric>

#include "ch04_stl_containers/order_book.h"
#include "ch04_stl_containers/portfolio.h"
#include "ch04_stl_containers/rolling_window.h"

int main() {
  using namespace cpp4q::hanson::ch04;  // NOLINT(build/namespaces) demo only

  OrderBook book;
  book.Update(Side::kBid, 99.5, 300);
  book.Update(Side::kBid, 99.0, 500);
  book.Update(Side::kAsk, 100.5, 200);
  book.Update(Side::kAsk, 101.0, 400);
  std::cout << "mid " << *book.Mid() << '\n';
  for (const auto& fill : book.ExecuteMarket(Side::kBid, 350)) {
    std::cout << "filled " << fill.quantity << " @ " << fill.price << '\n';
  }

  Portfolio pf;
  pf.Trade("AAPL", 100, 190.0);
  pf.Trade("MSFT", 50, 410.0);
  pf.Trade("AAPL", -40, 200.0);
  const PriceMap prices{{"AAPL", 195.0}, {"MSFT", 405.0}};
  std::cout << "MV " << pf.MarketValue(prices) << " realised "
            << pf.realized_pnl() << " unrealised " << pf.UnrealizedPnl(prices)
            << '\n';

  RollingWindow window(3);
  for (double x : {1.0, 2.0, 3.0, 4.0}) window.Push(x);
  std::cout << "window sum via iterators "
            << std::accumulate(window.begin(), window.end(), 0.0) << ", mean "
            << window.Mean() << '\n';
}
