#include "ch05_memory_model/atomic_stats.h"

#include <algorithm>
#include <thread>

namespace cpp4q::williams::ch05 {
namespace {

std::size_t BinOf(double x, double lo, double width, std::size_t num_bins) {
  const double pos = (x - lo) / width;
  if (!(pos > 0.0)) return 0;  // also catches NaN
  return std::min(static_cast<std::size_t>(pos), num_bins - 1);
}

}  // namespace

std::vector<std::int64_t> PnlHistogram(std::span<const double> pnl, double lo,
                                       double hi, std::size_t num_bins) {
  std::vector<std::int64_t> bins(num_bins, 0);
  if (num_bins == 0) return bins;
  const double width = (hi - lo) / static_cast<double>(num_bins);
  for (double x : pnl) ++bins[BinOf(x, lo, width, num_bins)];
  return bins;
}

std::vector<std::int64_t> ParallelPnlHistogram(std::span<const double> pnl,
                                               double lo, double hi,
                                               std::size_t num_bins,
                                               unsigned num_threads) {
  std::vector<std::int64_t> bins(num_bins, 0);
  if (num_bins == 0) return bins;
  if (num_threads == 0) num_threads = 1;
  const double width = (hi - lo) / static_cast<double>(num_bins);
  {
    std::vector<std::jthread> workers;
    for (unsigned t = 0; t < num_threads; ++t) {
      workers.emplace_back([&, t] {
        const std::size_t begin = pnl.size() * t / num_threads;
        const std::size_t end = pnl.size() * (t + 1) / num_threads;
        // Count privately, then publish: one atomic add per bin per thread
        // instead of one per sample. The first version did a shared
        // fetch_add per sample and ran 9x slower than the serial loop,
        // because PnL clusters in the middle bins and every core fought
        // over the same few cache lines (profiling/REPORT.md, finding 3).
        std::vector<std::int64_t> local(num_bins, 0);
        for (std::size_t i = begin; i < end; ++i) {
          ++local[BinOf(pnl[i], lo, width, num_bins)];
        }
        for (std::size_t b = 0; b < num_bins; ++b) {
          if (local[b] != 0) {
            std::atomic_ref<std::int64_t>(bins[b]).fetch_add(
                local[b], std::memory_order_relaxed);
          }
        }
      });
    }
  }  // join: happens-before the plain reads by the caller
  return bins;
}

}  // namespace cpp4q::williams::ch05
