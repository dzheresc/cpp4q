// Chapter 8: Pricing American options.
//
// The Cox-Ross-Rubinstein binomial tree: not Monte Carlo, but the
// reference that the simulation estimators of this chapter are measured
// against in one dimension.

#ifndef CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LATTICE_H_
#define CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LATTICE_H_

#include <cstddef>

#include "ch01_foundations/black_scholes.h"

namespace cpp4q::glasserman::ch08 {

using ch01::BlackScholesInputs;
using ch01::OptionType;

enum class ExerciseStyle { kEuropean, kAmerican, kBermudan };

// For kBermudan, exercise is allowed at j T / exercise_dates,
// j = 1..exercise_dates; steps must be a multiple of exercise_dates.
double CrrPrice(const BlackScholesInputs& in, OptionType type,
                std::size_t steps, ExerciseStyle style,
                std::size_t exercise_dates = 0);

}  // namespace cpp4q::glasserman::ch08

#endif  // CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LATTICE_H_
