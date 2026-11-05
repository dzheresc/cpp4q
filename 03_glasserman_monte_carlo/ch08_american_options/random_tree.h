// Chapter 8: Pricing American options.
//
// Broadie and Glasserman's random tree (8.3): from each node, simulate b
// independent successors at the next exercise date. Two estimators come
// from the same tree:
//   high - at each node, max(exercise, mean of discounted child values);
//          the max of an average is biased high (Jensen);
//   low  - decide whether to exercise using b - 1 children and value the
//          continuation with the one left out; the decision and the value
//          are independent, so this is biased low.
// Both converge as b grows, and together they bracket the true price.
// Cost is b^m per tree for m exercise dates.

#ifndef CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_RANDOM_TREE_H_
#define CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_RANDOM_TREE_H_

#include <cstddef>
#include <cstdint>

#include "ch01_foundations/statistics.h"
#include "ch08_american_options/lsm.h"

namespace cpp4q::glasserman::ch08 {

struct RandomTreeResult {
  ch01::Estimate high;
  ch01::Estimate low;
};

RandomTreeResult RandomTreePrice(const BermudanSpec& spec,
                                 std::size_t branching, std::size_t trees,
                                 std::uint64_t seed);

}  // namespace cpp4q::glasserman::ch08

#endif  // CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_RANDOM_TREE_H_
