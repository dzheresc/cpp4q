// Chapter 15 demo: logging, timing and a precondition failure that says
// where it happened.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <source_location>

#include "ch15_debugging_tools/debug.h"

namespace d = cpp4q::armstrong::ch15;

namespace {

double Volatility(double variance) {
  d::Require(variance >= 0, "variance must be non-negative");
  return std::sqrt(variance);
}

}  // namespace

int main() {
  d::Logger log(std::cout, d::LogLevel::kInfo);
  log.Log(d::LogLevel::kDebug, std::source_location::current(),
          "not shown: below threshold");
  log.Log(d::LogLevel::kInfo, std::source_location::current(),
          "vol from variance 0.04 = {}", Volatility(0.04));
  double sum = 0;
  {
    const d::ScopedTimer timer([&](auto elapsed) {
      log.Log(d::LogLevel::kInfo, std::source_location::current(),
              "summed 1e6 square roots in {} us",
              std::chrono::duration_cast<std::chrono::microseconds>(elapsed)
                  .count());
    });
    for (int i = 0; i < 1'000'000; ++i) sum += std::sqrt(i);
  }
  try {
    Volatility(-1);
  } catch (const d::PreconditionError& e) {
    log.Log(d::LogLevel::kError, std::source_location::current(),
            "caught: {}", e.what());
  }
  std::printf("(sum %.0f)\n", sum);
  return 0;
}
