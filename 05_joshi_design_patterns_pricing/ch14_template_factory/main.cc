// Chapter 14 demo: products and generators chosen by name, as from a
// configuration file.

#include <cstdio>
#include <string>

#include "ch05_strategies_statistics/statistics.h"
#include "ch06_random_numbers/random.h"
#include "ch14_template_factory/registrations.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c14 = j::ch14;
  const j::ch04::VanillaOption option(
      j::ch04::PayOffBridge(
          c14::RangePayOffFactory::Instance().Create("double_digital", 90, 110)),
      1.0);
  for (const std::string& name : c14::RandomFactory::Instance().Keys()) {
    auto gen = c14::RandomFactory::Instance().Create(name, 1, 1);
    j::ch05::StatisticsMean mean;
    j::ch06::SimpleMonteCarlo6(option, 100, 0.2, 0.05, 1 << 16, mean, *gen);
    std::printf("%-24s %.5f\n", name.c_str(), mean.GetResultsSoFar()[0][0]);
  }
}
