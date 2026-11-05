// Chapter 3: Generating sample paths.
//
// Geometric Brownian motion is simulated exactly on any grid by
// exponentiating Brownian increments (3.2.1); no discretization error.
// MultiAssetGbm correlates the drivers through a Cholesky factor (3.2.3).

#ifndef CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_GBM_H_
#define CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_GBM_H_

#include <cstddef>
#include <span>
#include <vector>

namespace cpp4q::glasserman::ch03 {

struct GbmParams {
  double spot = 100.0;
  double drift = 0.05;  // r - q under the risk-neutral measure
  double vol = 0.2;
};

// s[i] = S(t_i) given Brownian values w[i] = W(t_i).
void GbmFromBrownian(const GbmParams& p, std::span<const double> times,
                     std::span<const double> w, std::span<double> s);

// s[i] = S(t_i) from independent normals (random-walk construction).
void GbmPath(const GbmParams& p, std::span<const double> times,
             std::span<const double> z, std::span<double> s);

class MultiAssetGbm {
 public:
  MultiAssetGbm(std::vector<GbmParams> assets,
                std::span<const double> correlation);

  std::size_t dim() const { return assets_.size(); }

  // s[j] = S_j(t) from dim() independent normals z.
  void Terminal(double t, std::span<const double> z, std::span<double> s) const;

  // Path on a grid: z and s are times.size() x dim(), row-major by time.
  void Path(std::span<const double> times, std::span<const double> z,
            std::span<double> s) const;

 private:
  std::vector<GbmParams> assets_;
  std::vector<double> chol_;
};

}  // namespace cpp4q::glasserman::ch03

#endif  // CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_GBM_H_
