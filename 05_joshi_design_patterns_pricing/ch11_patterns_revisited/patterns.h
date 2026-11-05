// Chapter 11: Design patterns revisited.
//
// The book's catalogue so far: virtual constructor (ch 4), bridge (ch 4),
// strategy (ch 5, 6), decorator (ch 5, 6), template method (ch 7, 8),
// factory and singleton (ch 10). This chapter adds the structural patterns
// that were missing, each in pricing terms:
//
//   Adapter    RandomStdAdapter<Engine> makes any standard library engine
//              (std::mt19937_64, std::ranlux48, ...) a chapter 6 RandomBase,
//              so the engines of ch 7 run on it unchanged.
//   Composite  PayOffPortfolio is a PayOff made of weighted PayOffs, so a
//              call spread, a butterfly or a strangle is priced by any
//              pricer that takes one pay-off, in one simulation.
//   Decorator  PayOffCollar clamps another pay-off between a floor and a cap.

#ifndef CPP4Q_JOSHI_CH11_PATTERNS_REVISITED_PATTERNS_H_
#define CPP4Q_JOSHI_CH11_PATTERNS_REVISITED_PATTERNS_H_

#include <cstdint>
#include <memory>
#include <random>
#include <span>
#include <utility>
#include <vector>

#include "ch01_simple_monte_carlo/simple_mc.h"
#include "ch04_virtual_constructor/payoff3.h"
#include "ch04_virtual_constructor/payoff_bridge.h"
#include "ch06_random_numbers/random.h"

namespace cpp4q::joshi::ch11 {

template <std::uniform_random_bit_generator Engine>
class RandomStdAdapter final : public ch06::RandomBase {
 public:
  RandomStdAdapter(std::size_t dimensionality, std::uint64_t seed)
      : RandomBase(dimensionality), engine_(seed), seed_(seed) {}

  std::unique_ptr<ch06::RandomBase> Clone() const override {
    return std::make_unique<RandomStdAdapter>(*this);
  }
  void GetUniforms(std::span<double> variates) override {
    for (double& u : variates) u = ToOpenUnit(engine_());
  }
  void Skip(std::uint64_t number_of_paths) override {
    engine_.discard(number_of_paths * Dimensionality());
  }
  void SetSeed(std::uint64_t seed) override {
    seed_ = seed;
    engine_.seed(static_cast<typename Engine::result_type>(seed));
  }
  void Reset() override {
    engine_.seed(static_cast<typename Engine::result_type>(seed_));
  }

 private:
  // One engine output per uniform, so Skip is a single discard(). Uses the
  // top 53 bits (or all bits of a narrower engine), offset by half a step.
  static double ToOpenUnit(typename Engine::result_type x) {
    constexpr double kRange =
        static_cast<double>(Engine::max() - Engine::min()) + 1.0;
    if constexpr (Engine::max() - Engine::min() >= (1ULL << 53)) {
      return (static_cast<double>((x - Engine::min()) >> 11) + 0.5) *
             (2048.0 / kRange);
    } else {
      return (static_cast<double>(x - Engine::min()) + 0.5) / kRange;
    }
  }

  Engine engine_;
  std::uint64_t seed_;
};

// Composite: sum_i weight_i * payoff_i(S).
class PayOffPortfolio final : public ch04::Cloneable<PayOffPortfolio> {
 public:
  PayOffPortfolio() = default;
  PayOffPortfolio& Add(double weight, ch04::PayOffBridge payoff);
  double operator()(double spot) const override;
  std::size_t size() const { return legs_.size(); }

 private:
  std::vector<std::pair<double, ch04::PayOffBridge>> legs_;
};

// Decorator: min(max(inner(S), floor), cap).
class PayOffCollar final : public ch04::Cloneable<PayOffCollar> {
 public:
  // Throws unless floor <= cap.
  PayOffCollar(ch04::PayOffBridge inner, double floor, double cap);
  double operator()(double spot) const override;

 private:
  ch04::PayOffBridge inner_;
  double floor_;
  double cap_;
};

// Butterfly around `center`: calls at center -+ wing, minus two at center.
PayOffPortfolio MakeButterfly(double center, double wing);

}  // namespace cpp4q::joshi::ch11

#endif  // CPP4Q_JOSHI_CH11_PATTERNS_REVISITED_PATTERNS_H_
