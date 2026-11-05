#include "ch02_managing_threads/parallel_accumulate.h"

#include <algorithm>
#include <ranges>

namespace cpp4q::williams::ch02 {

unsigned ChooseThreadCount(std::size_t num_items, std::size_t min_per_thread,
                           unsigned max_threads) {
  if (num_items == 0) return 1;
  const std::size_t per = std::max<std::size_t>(min_per_thread, 1);
  const std::size_t wanted = (num_items + per - 1) / per;
  unsigned hw = std::thread::hardware_concurrency();
  if (hw == 0) hw = 2;
  if (max_threads != 0) hw = max_threads;
  return static_cast<unsigned>(
      std::clamp<std::size_t>(wanted, 1, static_cast<std::size_t>(hw)));
}

double PortfolioPnl(std::span<const Position> positions, unsigned max_threads) {
  auto pnl = positions | std::views::transform([](const Position& p) {
               return p.quantity * (p.mark_price - p.entry_price);
             });
  // transform_view over a span is random access, so the template applies.
  return ParallelAccumulate(pnl.begin(), pnl.end(), 0.0, 10'000, max_threads);
}

}  // namespace cpp4q::williams::ch02
