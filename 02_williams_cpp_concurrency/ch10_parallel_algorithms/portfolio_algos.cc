#include "ch10_parallel_algorithms/portfolio_algos.h"

#include <random>

namespace cpp4q::williams::ch10 {

std::vector<Bond> MakeBondBook(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> years(1, 30);
  std::uniform_int_distribution<int> lots(1, 500);
  std::uniform_real_distribution<double> coupon(0.0, 0.08);
  std::vector<Bond> book(n);
  for (std::size_t i = 0; i < n; ++i) {
    book[i] = {.id = static_cast<std::int64_t>(i),
               .notional = 1e5 * lots(rng),
               .coupon = coupon(rng),
               .years = years(rng)};
  }
  return book;
}

bool ParallelBackendAvailable() {
#if defined(CPP4Q_HAVE_TBB)
  return true;
#else
  return false;
#endif
}

}  // namespace cpp4q::williams::ch10
