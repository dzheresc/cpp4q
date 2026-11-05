// Chapter 3 demo: Brownian paths two ways, exact GBM, exact Vasicek and
// CIR bond prices, and Merton jump-diffusion against its series formula.

#include <cstdio>
#include <vector>

#include "ch02_random_numbers/normal.h"
#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/gbm.h"
#include "ch03_sample_paths/jump_diffusion.h"
#include "ch03_sample_paths/short_rate.h"

namespace g = cpp4q::glasserman;

int main() {
  const auto grid = g::ch03::UniformGrid(1.0, 8);
  g::ch02::Sampler s(g::ch02::Mrg32k3a(3));
  std::vector<double> z(8), walk(8), bridge(8), spot(8);
  s.FillNormal(z);
  g::ch03::RandomWalkPath(grid, z, walk);
  g::ch03::BrownianBridge(grid).Build(z, bridge);
  g::ch03::GbmFromBrownian({}, grid, bridge, spot);
  std::printf("%6s %10s %10s %10s\n", "t", "walk", "bridge", "GBM(bridge)");
  for (std::size_t i = 0; i < grid.size(); ++i) {
    std::printf("%6.3f %10.4f %10.4f %10.4f\n", grid[i], walk[i], bridge[i],
                spot[i]);
  }

  const g::ch03::ShortRateParams vas{
      .kappa = 0.3, .theta = 0.05, .sigma = 0.02};
  const auto v = g::ch03::VasicekBondMc(vas, 0.03, 5.0, 10, 50'000, s);
  std::printf("\nVasicek 5y ZCB: exact %.6f, MC %.6f +- %.6f\n",
              g::ch03::VasicekBondPrice(vas, 0.03, 5.0), v.mean, v.HalfWidth());
  const g::ch03::ShortRateParams cir{.kappa = 0.3, .theta = 0.05, .sigma = 0.1};
  const auto c = g::ch03::CirBondMc(cir, 0.03, 5.0, 50, 20'000, s);
  std::printf("CIR 5y ZCB:     exact %.6f, MC %.6f +- %.6f\n",
              g::ch03::CirBondPrice(cir, 0.03, 5.0), c.mean, c.HalfWidth());

  const g::ch03::MertonParams m{};
  const auto mc = g::ch03::MertonCallMc(m, 100.0, 1.0, 200'000, s);
  std::printf("Merton call:    series %.4f, MC %.4f +- %.4f\n",
              g::ch03::MertonCallPrice(m, 100.0, 1.0), mc.mean, mc.HalfWidth());
  return 0;
}
