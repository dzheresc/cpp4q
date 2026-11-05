// Chapter 5 demo: using the vector half of matlib from another file.

#include <cstdio>

#include "ch05_multiple_files/vectors.h"

namespace a = cpp4q::armstrong::ch05;

int main() {
  a::Rng rng(5);
  const auto z = a::RandN(100'000, rng);
  std::printf("100000 normals: mean %.4f, std %.4f, min %.3f, max %.3f\n",
              a::Mean(z), a::StandardDeviation(z), a::Min(z), a::Max(z));
  std::printf("percentiles 2.5 / 50 / 97.5: %.4f %.4f %.4f "
              "(theory -1.96 0 1.96)\n",
              a::Prctile(z, 2.5), a::Prctile(z, 50), a::Prctile(z, 97.5));
  const auto grid = a::Linspace(0.0, 1.0, 5);
  std::printf("linspace(0, 1, 5):");
  for (double x : grid) std::printf(" %.2f", x);
  std::printf("\n");
  return 0;
}
