// Chapter 2 demo: sizes and limits of the built-in types, overflow,
// integer division, machine epsilon, cancellation and summation error.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

#include "ch02_data_types/numerics.h"

namespace a = cpp4q::armstrong::ch02;

int main() {
  std::printf("sizeof: char %zu, int %zu, long %zu, long long %zu, float %zu, "
              "double %zu, long double %zu\n",
              sizeof(char), sizeof(int), sizeof(long), sizeof(long long),
              sizeof(float), sizeof(double), sizeof(long double));
  std::printf("int max %d, int64 max %lld, double max %g, denorm min %g\n",
              std::numeric_limits<int>::max(),
              static_cast<long long>(std::numeric_limits<std::int64_t>::max()),
              std::numeric_limits<double>::max(),
              std::numeric_limits<double>::denorm_min());

  const auto big = std::numeric_limits<std::int64_t>::max();
  std::printf("\nint64 max + 1 %s\n",
              a::CheckedAdd(big, 1) ? "fits" : "overflows (checked)");
  std::printf("-7 / 2 = %lld (truncates), floor div = %lld, -7 %% 2 = %lld, "
              "floor mod = %lld\n",
              static_cast<long long>(-7 / 2),
              static_cast<long long>(a::FloorDiv(-7, 2)),
              static_cast<long long>(-7 % 2),
              static_cast<long long>(a::FloorMod(-7, 2)));

  std::printf("\nmachine epsilon: float %g, double %g, long double %Lg\n",
              static_cast<double>(a::MachineEpsilon<float>()),
              a::MachineEpsilon<double>(), a::MachineEpsilon<long double>());

  const double x = 1e-10;
  std::printf("exp(x)-1 at x=1e-10: naive %.17g, expm1 %.17g\n",
              a::NaiveExpMinusOne(x), std::expm1(x));

  std::vector<double> tenths(10'000'000, 0.1);
  std::printf("sum of 1e7 x 0.1: naive %.10f, compensated %.10f\n",
              a::NaiveSum(tenths), a::CompensatedSum(tenths));
  return 0;
}
