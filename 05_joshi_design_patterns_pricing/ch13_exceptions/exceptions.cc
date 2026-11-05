#include "ch13_exceptions/exceptions.h"

#include <cmath>

namespace cpp4q::joshi::ch13 {

void ValidateBsInputs(double spot, double strike, double vol, double expiry) {
  if (!(spot > 0.0) || !std::isfinite(spot)) {
    throw InvalidInput("spot must be positive and finite");
  }
  if (!(strike > 0.0) || !std::isfinite(strike)) {
    throw InvalidInput("strike must be positive and finite");
  }
  if (!(vol >= 0.0) || !std::isfinite(vol)) {
    throw InvalidInput("vol must be non-negative and finite");
  }
  if (!(expiry >= 0.0) || !std::isfinite(expiry)) {
    throw InvalidInput("expiry must be non-negative and finite");
  }
}

void OptionBook::AddAllBasic(const std::vector<ch04::VanillaOption>& batch) {
  for (const ch04::VanillaOption& option : batch) options_.push_back(option);
}

void OptionBook::AddAllStrong(const std::vector<ch04::VanillaOption>& batch) {
  OptionBook copy = *this;  // may throw: *this untouched
  copy.options_.reserve(options_.size() + batch.size());
  copy.AddAllBasic(batch);  // may throw: only the copy is affected
  swap(*this, copy);        // cannot throw
}

double BumpedDelta(Market& market, double h,
                   const std::function<double(const Market&)>& price) {
  const double original = market.spot;
  const ScopeExit restore([&market, original]() noexcept {
    market.spot = original;
  });
  market.spot = original + h;
  const double up = price(market);
  market.spot = original - h;
  const double down = price(market);
  return (up - down) / (2.0 * h);
}

}  // namespace cpp4q::joshi::ch13
