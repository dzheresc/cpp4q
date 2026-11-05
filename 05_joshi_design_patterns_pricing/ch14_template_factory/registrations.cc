// Registrations into the chapter 14 factories, run at start-up. The library
// is linked whole-archive (see CMakeLists.txt) so these are kept.

#include "ch14_template_factory/registrations.h"

#include <random>

#include "ch05_strategies_statistics/wrapper.h"
#include "ch11_patterns_revisited/patterns.h"

namespace cpp4q::joshi::ch14 {
namespace {

const Registrar<ch04::PayOff, ch04::PayOffCall, double> kCall("call");
const Registrar<ch04::PayOff, ch04::PayOffPut, double> kPut("put");
const Registrar<ch04::PayOff, ch04::PayOffDigitalCall, double> kDigital(
    "digital_call");
const Registrar<ch04::PayOff, ch04::PayOffDoubleDigital, double, double>
    kDoubleDigital("double_digital");

using Mt = ch11::RandomStdAdapter<std::mt19937_64>;
const Registrar<ch06::RandomBase, ch06::RandomParkMiller, std::size_t,
                std::uint64_t>
    kParkMiller("park_miller");
const Registrar<ch06::RandomBase, Mt, std::size_t, std::uint64_t> kMt(
    "mt19937_64");

// Creators that do not fit a constructor: plain lambdas.
const bool kOthers = [] {
  auto& f = RandomFactory::Instance();
  f.Register("halton", [](std::size_t dim, std::uint64_t) {
    return std::make_unique<ch06::RandomHalton>(dim);
  });
  f.Register("antithetic_park_miller", [](std::size_t dim, std::uint64_t s) {
    return std::make_unique<ch06::AntiThetic>(
        Wrapper<ch06::RandomBase>(ch06::RandomParkMiller(dim, s)));
  });
  f.Register("antithetic_mt19937_64", [](std::size_t dim, std::uint64_t s) {
    return std::make_unique<ch06::AntiThetic>(Wrapper<ch06::RandomBase>(Mt(dim, s)));
  });
  return true;
}();

}  // namespace
}  // namespace cpp4q::joshi::ch14
