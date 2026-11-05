#include "ch03_sharing_data/quote_cache.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace cpp4q::williams::ch03 {

void QuoteCache::Update(std::string_view symbol, const Quote& quote) {
  std::unique_lock lock(mutex_);
  if (auto it = quotes_.find(symbol); it != quotes_.end()) {
    it->second = quote;
  } else {
    quotes_.emplace(std::string(symbol), quote);
  }
}

std::optional<Quote> QuoteCache::Get(std::string_view symbol) const {
  std::shared_lock lock(mutex_);
  if (auto it = quotes_.find(symbol); it != quotes_.end()) return it->second;
  return std::nullopt;
}

std::size_t QuoteCache::size() const {
  std::shared_lock lock(mutex_);
  return quotes_.size();
}

double ZeroCurve::Discount(double t) const {
  if (tenors.empty()) return 1.0;
  double z;
  if (t <= tenors.front()) {
    z = zero_rates.front();
  } else if (t >= tenors.back()) {
    z = zero_rates.back();
  } else {
    const auto hi = std::upper_bound(tenors.begin(), tenors.end(), t);
    const auto i = static_cast<std::size_t>(std::distance(tenors.begin(), hi));
    const double w = (t - tenors[i - 1]) / (tenors[i] - tenors[i - 1]);
    z = zero_rates[i - 1] + w * (zero_rates[i] - zero_rates[i - 1]);
  }
  return std::exp(-z * t);
}

}  // namespace cpp4q::williams::ch03
