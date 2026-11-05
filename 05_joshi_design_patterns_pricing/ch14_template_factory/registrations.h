// Chapter 14: the factories this book's classes are registered in.

#ifndef CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_REGISTRATIONS_H_
#define CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_REGISTRATIONS_H_

#include <cstdint>

#include "ch04_virtual_constructor/payoff3.h"
#include "ch06_random_numbers/random.h"
#include "ch14_template_factory/factory.h"

namespace cpp4q::joshi::ch14 {

// "call", "put", "digital_call" from a strike.
using StrikePayOffFactory = Factory<ch04::PayOff, double>;
// "double_digital" from (lower, upper).
using RangePayOffFactory = Factory<ch04::PayOff, double, double>;
// "park_miller", "mt19937_64", "halton" (seed ignored), "antithetic_*"
// from (dimensionality, seed).
using RandomFactory = Factory<ch06::RandomBase, std::size_t, std::uint64_t>;

}  // namespace cpp4q::joshi::ch14

#endif  // CPP4Q_JOSHI_CH14_TEMPLATE_FACTORY_REGISTRATIONS_H_
