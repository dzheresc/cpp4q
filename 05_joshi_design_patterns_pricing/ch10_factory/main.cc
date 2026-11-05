// Chapter 10 demo: pay-offs created by name, as from user input.

#include <initializer_list>
#include <cstdio>
#include <exception>
#include <string>

#include "ch05_strategies_statistics/statistics.h"
#include "ch10_factory/payoff_factory.h"

int main() {
  namespace j = cpp4q::joshi;
  auto& factory = j::ch10::PayOffFactory::Instance();
  std::printf("registered:");
  for (const std::string& id : factory.RegisteredIds()) {
    std::printf(" %s", id.c_str());
  }
  std::printf("\n");
  for (const char* id : {"call", "put", "straddle", "digital_call", "swap"}) {
    try {
      const j::ch04::VanillaOption option(
          j::ch04::PayOffBridge(factory.CreatePayOff(id, 100)), 1.0);
      j::ch05::StatisticsMean mean;
      j::Rng rng(10);
      j::ch05::SimpleMonteCarlo5(option, 100, 0.2, 0.05, 100'000, mean, rng);
      std::printf("%-13s %.4f\n", id, mean.GetResultsSoFar()[0][0]);
    } catch (const std::exception& e) {
      std::printf("%-13s error: %s\n", id, e.what());
    }
  }
}
