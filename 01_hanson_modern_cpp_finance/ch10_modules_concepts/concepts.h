// Chapter 10: Modules and Concepts.
//
// Concepts name the requirements that generic pricing code places on its
// arguments, turning template error novels into one-line diagnostics.

#ifndef CPP4Q_HANSON_CH10_MODULES_CONCEPTS_CONCEPTS_H_
#define CPP4Q_HANSON_CH10_MODULES_CONCEPTS_CONCEPTS_H_

#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>

namespace cpp4q::hanson::ch10 {

template <typename T>
concept Real = std::floating_point<T>;

// Anything callable as payoff(spot) -> real: lambdas, function pointers,
// the ch03 Payoff hierarchy, the ch03 variant alternatives...
template <typename F, typename T = double>
concept PayoffFunction =
    Real<T> && std::regular_invocable<const F&, T> &&
    std::convertible_to<std::invoke_result_t<const F&, T>, T>;

// Anything that can discount: ch03::YieldCurve satisfies it, as does a
// three-line struct in a unit test.
template <typename C>
concept DiscountCurve = requires(const C& curve, double t) {
  { curve.DiscountFactor(t) } -> std::convertible_to<double>;
};

// A dated cash flow: time in years and an amount.
template <typename T>
concept TimedCashFlow = requires(const T& cf) {
  { cf.time } -> std::convertible_to<double>;
  { cf.amount } -> std::convertible_to<double>;
};

template <typename R>
concept CashFlowRange =
    std::ranges::input_range<R> && TimedCashFlow<std::ranges::range_value_t<R>>;

}  // namespace cpp4q::hanson::ch10

#endif  // CPP4Q_HANSON_CH10_MODULES_CONCEPTS_CONCEPTS_H_
