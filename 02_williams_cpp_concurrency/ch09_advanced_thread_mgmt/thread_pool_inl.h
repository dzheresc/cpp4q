// Template definitions for thread_pool.h.

#ifndef CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_INL_H_
#define CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_INL_H_

#include <algorithm>

namespace cpp4q::williams::ch09 {
namespace internal {

template <typename T>
void PoolQuickSortImpl(ThreadPool& pool, T* first, T* last,
                       std::size_t cutoff) {
  while (static_cast<std::size_t>(last - first) > cutoff) {
    const std::size_t n = static_cast<std::size_t>(last - first);
    const T a = first[0], b = first[n / 2], c = last[-1];
    const T pivot = std::max(std::min(a, b), std::min(std::max(a, b), c));
    T* mid1 =
        std::partition(first, last, [&](const T& x) { return x < pivot; });
    T* mid2 =
        std::partition(mid1, last, [&](const T& x) { return !(pivot < x); });
    auto lower = pool.Submit([&pool, first, mid1, cutoff] {
      PoolQuickSortImpl(pool, first, mid1, cutoff);
    });
    PoolQuickSortImpl(pool, mid2, last, cutoff);
    pool.WaitFor(lower);
    return;
  }
  std::sort(first, last);
}

}  // namespace internal

template <typename T>
void PoolQuickSort(ThreadPool& pool, std::vector<T>& v,
                   std::size_t serial_cutoff) {
  if (v.empty()) return;
  internal::PoolQuickSortImpl(pool, v.data(), v.data() + v.size(),
                              std::max<std::size_t>(serial_cutoff, 16));
}

}  // namespace cpp4q::williams::ch09

#endif  // CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_INL_H_
