// Chapter 10: a C++20 named module.
//
// Built only with -DCPP4Q_ENABLE_MODULES=ON (CMake >= 3.28, Ninja, and
// clang >= 17 or gcc >= 14). Importers see only the exported names; the
// helper in the anonymous namespace is invisible to them, and the module is
// compiled once instead of re-parsed per translation unit.

module;

#include <algorithm>
#include <cmath>
#include <numbers>

export module cpp4q.hanson.pricing;

namespace {

double NormCdf(double x) { return 0.5 * std::erfc(-x / std::numbers::sqrt2); }

}  // namespace

export namespace cpp4q::hanson::modules {

enum class OptionType { kCall, kPut };

double BlackScholes(OptionType type, double spot, double strike, double rate,
                    double vol, double t) {
  const double vst = vol * std::sqrt(t);
  const double d1 = (std::log(spot / strike) + rate * t) / vst + 0.5 * vst;
  const double d2 = d1 - vst;
  const double df = std::exp(-rate * t);
  return type == OptionType::kCall
             ? spot * NormCdf(d1) - strike * df * NormCdf(d2)
             : strike * df * NormCdf(-d2) - spot * NormCdf(-d1);
}

}  // namespace cpp4q::hanson::modules
