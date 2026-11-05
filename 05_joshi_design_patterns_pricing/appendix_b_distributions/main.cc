// Appendix B demo: the Moro inverse against the exact cdf.

#include <initializer_list>
#include <cstdio>

#include "appendix_b_distributions/normals.h"

int main() {
  namespace j = cpp4q::joshi;
  std::printf("%10s %22s %22s %12s\n", "u", "Moro", "refined", "N(x)-u");
  for (const double u : {1e-10, 1e-6, 0.01, 0.1, 0.42, 0.5, 0.9, 0.999}) {
    const double x = j::InverseCumulativeNormal(u);
    std::printf("%10.3g %22.15f %22.15f %12.3e\n", u, x,
                j::InverseCumulativeNormalRefined(u),
                j::CumulativeNormal(x) - u);
  }
}
