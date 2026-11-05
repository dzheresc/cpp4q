// Chapter 10: concept-constrained generic pricing algorithms.

#ifndef CPP4Q_HANSON_CH10_MODULES_CONCEPTS_GENERIC_PRICING_H_
#define CPP4Q_HANSON_CH10_MODULES_CONCEPTS_GENERIC_PRICING_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "ch10_modules_concepts/concepts.h"

namespace cpp4q::hanson::ch10 {

// Net present value of any range of timed cash flows on any curve.
template <CashFlowRange R, DiscountCurve C>
double Npv(const R& flows, const C& curve) {
  double npv = 0.0;
  for (const auto& cf : flows) {
    npv += static_cast<double>(cf.amount) *
           curve.DiscountFactor(static_cast<double>(cf.time));
  }
  return npv;
}

enum class ExerciseStyle { kEuropean, kAmerican };

template <Real T>
struct TreeParams {
  T spot;
  T rate;
  T dividend_yield;
  T volatility;
  T expiry;
  std::size_t steps;
};

// Cox-Ross-Rubinstein binomial tree, generic over the scalar type and the
// payoff. O(steps^2) time, O(steps) memory.
template <Real T, PayoffFunction<T> F>
T PriceBinomial(const TreeParams<T>& p, const F& payoff, ExerciseStyle style) {
  if (p.steps == 0) throw std::invalid_argument("PriceBinomial: steps == 0");
  const T dt = p.expiry / static_cast<T>(p.steps);
  const T u = std::exp(p.volatility * std::sqrt(dt));
  const T d = T{1} / u;
  const T growth = std::exp((p.rate - p.dividend_yield) * dt);
  const T q = (growth - d) / (u - d);
  if (!(q > T{0} && q < T{1})) {
    throw std::invalid_argument(
        "PriceBinomial: arbitrage in tree (dt too big)");
  }
  const T disc = std::exp(-p.rate * dt);

  // Node j at step n has spot S u^(2j - n). Walk spots multiplicatively
  // (one multiply per node) instead of calling std::pow per node - pow was
  // ~80% of the American-exercise runtime under callgrind.
  const T u2 = u * u;
  std::vector<T> values(p.steps + 1);
  T s = p.spot * std::pow(d, static_cast<T>(p.steps));
  for (std::size_t j = 0; j <= p.steps; ++j, s *= u2) {
    values[j] = static_cast<T>(payoff(s));
  }
  const T q_up = disc * q;
  const T q_down = disc * (T{1} - q);
  T lowest = p.spot * std::pow(d, static_cast<T>(p.steps));
  for (std::size_t n = p.steps; n-- > 0;) {
    lowest *= u;  // S d^n, the lowest node of step n
    if (style == ExerciseStyle::kAmerican) {
      T node = lowest;
      for (std::size_t j = 0; j <= n; ++j, node *= u2) {
        const T cont = q_up * values[j + 1] + q_down * values[j];
        values[j] = std::max(cont, static_cast<T>(payoff(node)));
      }
    } else {
      for (std::size_t j = 0; j <= n; ++j) {
        values[j] = q_up * values[j + 1] + q_down * values[j];
      }
    }
  }
  return values[0];
}

// Abbreviated function template syntax: `auto` constrained by a concept.
inline double PayoffAt(const PayoffFunction auto& payoff, double spot) {
  return payoff(spot);
}

}  // namespace cpp4q::hanson::ch10

#endif  // CPP4Q_HANSON_CH10_MODULES_CONCEPTS_GENERIC_PRICING_H_
