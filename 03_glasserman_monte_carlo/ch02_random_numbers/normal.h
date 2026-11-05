// Chapter 2: Generating random numbers and random variables.
//
// Normal variates three ways: inversion with the Beasley-Springer-Moro
// approximation of the inverse normal CDF (the method of choice for
// stratification and quasi-Monte Carlo, because it is monotone in u),
// Box-Muller, and Marsaglia's polar rejection method. Sampler bundles a
// Mrg32k3a with a normal method; the later chapters draw from it.

#ifndef CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_NORMAL_H_
#define CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_NORMAL_H_

#include <cmath>
#include <span>

#include "ch02_random_numbers/rng.h"

namespace cpp4q::glasserman::ch02 {

namespace internal {

inline constexpr double kBsmA[4] = {2.50662823884, -18.61500062529,
                                    41.39119773534, -25.44106049637};
inline constexpr double kBsmB[4] = {-8.47351093090, 23.08336743743,
                                    -21.06224101826, 3.13082909833};
inline constexpr double kBsmC[9] = {
    0.3374754822726147, 0.9761690190917186, 0.1607979714918209,
    0.0276438810333863, 0.0038405729373609, 0.0003951896511919,
    0.0000321767881768, 0.0000002888167364, 0.0000003960315187};

}  // namespace internal

// Beasley-Springer-Moro; absolute error below 3e-9 on (1e-10, 1 - 1e-10).
// Inline because it runs once per normal draw.
inline double InverseNormalCdf(double u) {
  using internal::kBsmA;
  using internal::kBsmB;
  using internal::kBsmC;
  const double y = u - 0.5;
  if (std::abs(y) < 0.42) {
    const double r = y * y;
    return y * (((kBsmA[3] * r + kBsmA[2]) * r + kBsmA[1]) * r + kBsmA[0]) /
           ((((kBsmB[3] * r + kBsmB[2]) * r + kBsmB[1]) * r + kBsmB[0]) * r +
            1.0);
  }
  double r = y > 0.0 ? 1.0 - u : u;
  r = std::log(-std::log(r));
  double x = kBsmC[8];
  for (int i = 7; i >= 0; --i) x = kBsmC[i] + r * x;
  return y < 0.0 ? -x : x;
}
// BSM followed by one Halley step on erfc: close to machine precision.
double InverseNormalCdfAccurate(double u);

enum class NormalMethod { kInversion, kBoxMuller, kMarsagliaPolar };

class Sampler {
 public:
  explicit Sampler(Mrg32k3a rng = Mrg32k3a(),
                   NormalMethod method = NormalMethod::kInversion)
      : rng_(rng), method_(method) {}

  double Uniform() { return rng_.NextUniform(); }
  double Normal() {
    return method_ == NormalMethod::kInversion
               ? InverseNormalCdf(rng_.NextUniform())
               : OtherNormal();
  }
  void FillNormal(std::span<double> out);

  Mrg32k3a& rng() { return rng_; }
  NormalMethod method() const { return method_; }

 private:
  double OtherNormal();
  double BoxMuller();
  double Polar();

  Mrg32k3a rng_;
  NormalMethod method_;
  bool has_spare_ = false;
  double spare_ = 0.0;
};

}  // namespace cpp4q::glasserman::ch02

#endif  // CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_NORMAL_H_
