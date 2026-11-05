#include "ch03_sample_paths/brownian.h"

#include <cmath>
#include <deque>
#include <stdexcept>
#include <utility>

namespace cpp4q::glasserman::ch03 {
namespace {

void CheckGrid(std::span<const double> times) {
  double prev = 0.0;
  for (double t : times) {
    if (!(t > prev)) {
      throw std::invalid_argument("times must be positive and increasing");
    }
    prev = t;
  }
}

}  // namespace

std::vector<double> UniformGrid(double maturity, std::size_t steps) {
  if (steps == 0 || !(maturity > 0.0)) {
    throw std::invalid_argument("need steps > 0 and maturity > 0");
  }
  std::vector<double> t(steps);
  for (std::size_t i = 0; i < steps; ++i) {
    t[i] = maturity * static_cast<double>(i + 1) / static_cast<double>(steps);
  }
  return t;
}

void RandomWalkPath(std::span<const double> times, std::span<const double> z,
                    std::span<double> w) {
  if (z.size() != times.size() || w.size() != times.size()) {
    throw std::invalid_argument("size mismatch");
  }
  double prev_t = 0.0;
  double prev_w = 0.0;
  for (std::size_t i = 0; i < times.size(); ++i) {
    prev_w += std::sqrt(times[i] - prev_t) * z[i];
    w[i] = prev_w;
    prev_t = times[i];
  }
}

BrownianBridge::BrownianBridge(std::vector<double> times)
    : times_(std::move(times)) {
  if (times_.empty()) throw std::invalid_argument("empty time grid");
  CheckGrid(times_);
  const std::size_t n = times_.size();
  steps_.reserve(n);
  // First: W(t_n) from W(0) = 0.
  steps_.push_back(
      {n - 1, 0, 0, false, false, 0.0, 0.0, std::sqrt(times_[n - 1])});
  // Intervals (left, right) in index space; left == n means "time 0".
  std::deque<std::pair<std::size_t, std::size_t>> queue;
  queue.emplace_back(n, n - 1);
  while (!queue.empty()) {
    const auto [l, r] = queue.front();
    queue.pop_front();
    const std::size_t first = (l == n) ? 0 : l + 1;  // first unknown index
    if (first >= r) continue;
    const std::size_t m = first + (r - first) / 2;
    const double tl = (l == n) ? 0.0 : times_[l];
    const double tm = times_[m];
    const double tr = times_[r];
    const double span = tr - tl;
    steps_.push_back({m, l == n ? 0 : l, r, l != n, true, (tr - tm) / span,
                      (tm - tl) / span,
                      std::sqrt((tm - tl) * (tr - tm) / span)});
    queue.emplace_back(l, m);
    queue.emplace_back(m, r);
  }
}

void BrownianBridge::Build(std::span<const double> z,
                           std::span<double> w) const {
  if (z.size() != times_.size() || w.size() != times_.size()) {
    throw std::invalid_argument("size mismatch");
  }
  for (std::size_t k = 0; k < steps_.size(); ++k) {
    const Step& s = steps_[k];
    double mean = 0.0;
    if (s.has_left) mean += s.left_weight * w[s.left];
    if (s.has_right) mean += s.right_weight * w[s.right];
    w[s.index] = mean + s.sd * z[k];
  }
}

}  // namespace cpp4q::glasserman::ch03
