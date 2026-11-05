// Chapter 7 demo: arithmetic and geometric Asians and a discrete knock-out
// through one engine.

#include <cstdio>
#include <vector>

#include "ch07_exotics_engine/exotic_engine.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  namespace c5 = j::ch05;
  namespace c6 = j::ch06;
  namespace c7 = j::ch07;
  const double spot = 100, r = 0.05, d = 0.0, vol = 0.2, expiry = 1;
  std::vector<double> times;
  for (int i = 1; i <= 12; ++i) times.push_back(expiry * i / 12.0);
  const c4::PayOffCall call(100);

  const auto run = [&](const c7::PathDependent& product, const char* name) {
    c7::ExoticBsEngine engine(product, r, d, vol,
                              c6::AntiThetic(c6::RandomParkMiller(1, 7)), spot);
    c5::StatisticsMoments stats;
    engine.DoSimulation(stats, 1 << 17);
    const auto row = stats.GetResultsSoFar()[0];
    std::printf("%-22s %.4f (+- %.4f naive)\n", name, row[0], row[1]);
  };
  run(c7::PathDependentAsian(times, expiry, call), "arithmetic Asian");
  run(c7::PathDependentGeometricAsian(times, expiry, call), "geometric Asian");
  std::printf("%-22s %.4f\n", "geometric closed form",
              c7::GeometricAsianCall(spot, 100, r, d, vol, times, expiry));
  run(c7::PathDependentKnockOut(times, 90, c7::BarrierDirection::kDown, call),
      "down-and-out (12 obs)");
  std::printf("%-22s %.4f\n", "continuous closed form",
              c7::DownAndOutCallContinuous(spot, 100, 90, r, d, vol, expiry));
}
