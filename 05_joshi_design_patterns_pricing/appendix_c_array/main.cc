// Appendix C demo: value semantics and element-wise arithmetic.

#include <cmath>
#include <cstdio>

#include "appendix_c_array/mj_array.h"

int main() {
  using cpp4q::joshi::MjArray;
  MjArray a{1, 2, 3, 4};
  MjArray b = a;  // deep copy
  b *= 2.0;
  b += a;  // 3, 6, 9, 12
  const MjArray c = (a + b) * 0.5;
  std::printf("sum(c) = %g, min = %g, max = %g\n", c.Sum(), c.Min(), c.Max());
  b.resize(2);
  b.resize(4);  // within capacity: no reallocation
  std::printf("b size %zu capacity %zu\n", b.size(), b.capacity());
  MjArray logs = a;
  logs.Apply([](double x) { return std::log(x); });
  std::printf("sum(log a) = log 24 = %.12f\n", logs.Sum());
}
