// Chapter 6 demo: serial vs parallel Monte Carlo.

#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <thread>
#include <utility>

#include "ch06_random_concurrency/monte_carlo.h"

int main() {
  namespace ch06 = cpp4q::hanson::ch06;
  const ch06::GbmParams gbm{.spot = 100.0,
                            .rate = 0.03,
                            .dividend_yield = 0.0,
                            .volatility = 0.2,
                            .expiry = 1.0};
  const ch06::McConfig config{.num_paths = 2'000'000, .seed = 7};
  const unsigned threads = std::max(1u, std::thread::hardware_concurrency());

  auto timed = [](auto&& fn) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto r = fn();
    const std::chrono::duration<double, std::milli> ms =
        std::chrono::steady_clock::now() - t0;
    return std::pair{r, ms.count()};
  };

  const auto [serial, t_serial] = timed([&] {
    return ch06::PriceEuropeanMc(ch06::OptionType::kCall, 100.0, gbm, config);
  });
  const auto [parallel, t_parallel] = timed([&] {
    return ch06::PriceEuropeanMcParallel(ch06::OptionType::kCall, 100.0, gbm,
                                         config, threads);
  });
  std::cout << std::format("serial   {:.4f} +/- {:.4f}  {:.1f} ms\n",
                           serial.price, serial.std_error, t_serial)
            << std::format("parallel {:.4f} +/- {:.4f}  {:.1f} ms ({} thr)\n",
                           parallel.price, parallel.std_error, t_parallel,
                           threads);

  const auto asian = ch06::PriceAsianMcParallel(
      ch06::OptionType::kCall, 100.0, gbm, 52, {.num_paths = 200'000}, threads);
  std::cout << std::format("asian (52 fixings) {:.4f} +/- {:.4f}\n",
                           asian.price, asian.std_error);
}
