// Chapter 1 demo: a first program. Prints how 1000 grows at 5% for ten
// years under each compounding convention.

#include <cstdio>
#include <initializer_list>

#include "ch01_getting_started/interest.h"

namespace a = cpp4q::armstrong::ch01;

int main() {
  constexpr double kPrincipal = 1000.0;
  constexpr double kRate = 0.05;
  constexpr double kYears = 10.0;
  std::printf("Hello, financial mathematics.\n\n");
  std::printf("%-22s %12s\n", "convention", "value");
  std::printf("%-22s %12.4f\n", "simple",
              a::SimpleInterest(kPrincipal, kRate, kYears));
  for (int m : {1, 2, 4, 12, 365}) {
    std::printf("compounded %3d / year  %12.4f\n", m,
                a::CompoundInterest(kPrincipal, kRate, kYears, m));
  }
  std::printf("%-22s %12.4f\n", "continuous",
              a::ContinuousCompounding(kPrincipal, kRate, kYears));
  std::printf("\n5%% annual = %.6f%% continuous; doubling time %.2f years\n",
              100 * a::ContinuousEquivalentRate(kRate, 1),
              a::DoublingTime(a::ContinuousEquivalentRate(kRate, 1)));
  return 0;
}
