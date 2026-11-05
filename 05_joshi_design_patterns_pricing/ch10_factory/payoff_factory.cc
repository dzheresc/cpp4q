#include "ch10_factory/payoff_factory.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch10 {

PayOffFactory& PayOffFactory::Instance() {
  static PayOffFactory the_factory;
  return the_factory;
}

bool PayOffFactory::RegisterPayOff(std::string id,
                                   CreatePayOffFunction creator) {
  return creators_.emplace(std::move(id), creator).second;
}

std::unique_ptr<ch04::PayOff> PayOffFactory::CreatePayOff(
    std::string_view id, double strike) const {
  const auto it = creators_.find(id);
  if (it == creators_.end()) {
    std::string known;
    for (const auto& [name, creator] : creators_) {
      known += known.empty() ? name : ", " + name;
    }
    throw std::out_of_range("PayOffFactory: unknown pay-off '" +
                            std::string(id) + "' (known: " + known + ")");
  }
  return it->second(strike);
}

std::vector<std::string> PayOffFactory::RegisteredIds() const {
  std::vector<std::string> ids;
  ids.reserve(creators_.size());
  for (const auto& [name, creator] : creators_) ids.push_back(name);
  return ids;
}

double PayOffStraddle::operator()(double spot) const {
  return std::fabs(spot - strike_);
}

}  // namespace cpp4q::joshi::ch10
