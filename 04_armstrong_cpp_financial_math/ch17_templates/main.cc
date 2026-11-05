// Chapter 17 demo: the same integrator for float, double and long double;
// compile-time matrix algebra; a zero-rate curve.

#include <array>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <numbers>
#include <vector>

#include "ch03_functions/matlib.h"
#include "ch17_templates/templates.h"

namespace a = cpp4q::armstrong;

int main() {
  const auto pdf_f = [](float x) { return std::exp(-0.5f * x * x); };
  const auto pdf_d = [](double x) { return a::ch03::NormPdf(x); };
  const auto pdf_l = [](long double x) { return std::exp(-0.5L * x * x); };
  std::printf("integral of the normal density over [-5, 5]:\n");
  std::printf("  float       %.9f\n",
              static_cast<double>(a::ch17::IntegrateSimpson(pdf_f, -5.0f, 5.0f,
                                                            200) /
                                  std::sqrt(2 * std::numbers::pi_v<float>)));
  std::printf("  double      %.15f\n",
              a::ch17::IntegrateSimpson(pdf_d, -5.0, 5.0, 200));
  std::printf("  long double %.18Lf\n",
              a::ch17::IntegrateSimpson(pdf_l, -5.0L, 5.0L, 200) /
                  std::sqrt(2 * std::numbers::pi_v<long double>));

  constexpr a::ch17::FixedMatrix<int, 2, 3> m(
      std::array<int, 6>{1, 2, 3, 4, 5, 6});
  constexpr auto mmt = m * m.Transpose();  // 2x2, computed at compile time
  static_assert(mmt(0, 0) == 14 && mmt(1, 1) == 77 && mmt(0, 1) == 32);
  std::printf("\nM M^T (compile time) = [[%d, %d], [%d, %d]]\n", mmt(0, 0),
              mmt(0, 1), mmt(1, 0), mmt(1, 1));

  const a::ch17::LinearInterpolator zero_curve(
      std::vector<double>{0.25, 1, 2, 5, 10},
      std::vector<double>{0.030, 0.034, 0.037, 0.040, 0.042});
  std::printf("\n%6s %10s %12s\n", "years", "zero rate", "discount");
  for (double t : {0.1, 0.5, 1.5, 3.0, 7.0, 20.0}) {
    std::printf("%6.2f %9.4f%% %12.6f\n", t, 100 * zero_curve(t),
                std::exp(-zero_curve(t) * t));
  }
  return 0;
}
