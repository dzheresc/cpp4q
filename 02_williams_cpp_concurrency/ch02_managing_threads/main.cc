// Chapter 2 demo: thread ownership, RAII joining, parallel accumulate.

#include <cstdio>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include "ch02_managing_threads/parallel_accumulate.h"
#include "ch02_managing_threads/thread_guards.h"

int main() {
  namespace ch02 = cpp4q::williams::ch02;

  // Ownership moves between std::thread objects; exactly one owns it.
  std::thread t1([] { std::puts("worker running"); });
  std::thread t2 = std::move(t1);
  std::printf("t1 joinable: %d, t2 joinable: %d\n", t1.joinable(),
              t2.joinable());
  ch02::ScopedThread guard(std::move(t2));  // joined at end of main

  std::vector<ch02::Position> book(1'000'000);
  std::mt19937_64 rng(7);
  std::normal_distribution<double> move(0.0, 1.0);
  for (auto& p : book) {
    p.quantity = 100.0;
    p.entry_price = 50.0;
    p.mark_price = 50.0 + move(rng);
  }
  std::printf("portfolio PnL over %zu positions on %u threads: %.2f\n",
              book.size(), ch02::ChooseThreadCount(book.size(), 10'000),
              ch02::PortfolioPnl(book));
}
