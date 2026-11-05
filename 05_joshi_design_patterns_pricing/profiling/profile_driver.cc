// Fixed, deterministic workloads for valgrind (memcheck, helgrind, drd,
// callgrind, cachegrind, massif). Sizes are small because valgrind runs
// 20-100x slower than native.
// Usage: joshi_profile <workload>; run without arguments to list.

#include <cstdio>
#include <cstring>
#include <functional>
#include <random>
#include <string_view>
#include <thread>
#include <vector>

#include "appendix_c_array/mj_array.h"
#include "ch07_exotics_engine/exotic_engine.h"
#include "ch08_trees/tree.h"
#include "ch09_solvers_implied_vol/solvers.h"
#include "ch12_situation_2007/modern.h"
#include "ch14_template_factory/registrations.h"
#include "ch16_decoupling/mc_pricer.h"

namespace {

namespace j = cpp4q::joshi;

double g_sink = 0.0;  // keeps results observable

std::vector<double> Times(int n) {
  std::vector<double> t;
  for (int i = 1; i <= n; ++i) t.push_back(static_cast<double>(i) / n);
  return t;
}

void Mc5() {
  const j::ch04::VanillaOption call(j::ch04::PayOffCall(100), 1);
  j::Rng rng(1);
  j::ch05::ConvergenceTable table{
      j::Wrapper<j::ch05::StatisticsMc>(j::ch05::StatisticsMoments())};
  j::ch05::SimpleMonteCarlo5(call, 100, 0.2, 0.05, 200'000, table, rng);
  g_sink += table.GetResultsSoFar().back()[0];
}

void Mc6() {
  const j::ch04::VanillaOption call(j::ch04::PayOffCall(100), 1);
  j::ch06::AntiThetic gen{
      j::Wrapper<j::ch06::RandomBase>(j::ch06::RandomParkMiller(1))};
  j::ch05::StatisticsMean mean;
  j::ch06::SimpleMonteCarlo6(call, 100, 0.2, 0.05, 200'000, mean, gen);
  g_sink += mean.GetResultsSoFar()[0][0];
}

void Asian() {
  const j::ch07::PathDependentAsian asian(Times(52), 1.0,
                                          j::ch04::PayOffCall(100));
  j::ch07::ExoticBsEngine engine(asian, 0.05, 0.0, 0.2,
                                 j::ch06::RandomParkMiller(1, 1), 100);
  j::ch05::StatisticsMean mean;
  engine.DoSimulation(mean, 10'000);
  g_sink += mean.GetResultsSoFar()[0][0];
}

void KnockOut() {
  const j::ch07::PathDependentKnockOut ko(Times(52), 85,
                                          j::ch07::BarrierDirection::kDown,
                                          j::ch04::PayOffCall(100));
  j::ch07::ExoticBsEngine engine(ko, 0.05, 0.0, 0.2,
                                 j::ch06::RandomParkMiller(1, 2), 100);
  j::ch05::StatisticsMoments stats;
  engine.DoSimulation(stats, 10'000);
  g_sink += stats.GetResultsSoFar()[0][0];
}

void Tree() {
  j::ch08::SimpleBinomialTree tree(100, 0.05, 0.02, 0.2, 1000, 1.0);
  g_sink += tree.GetThePrice(j::ch08::TreeAmerican(1.0, j::ch04::PayOffPut(100)));
  g_sink += tree.GetThePrice(j::ch08::TreeEuropean(1.0, j::ch04::PayOffCall(100)));
}

void ImpliedVol() {
  for (int i = 0; i < 2000; ++i) {
    const j::ch09::BsCall call(0.05, 0.0, 1.0, 100, 60 + 0.04 * i);
    const double price = call.Price(0.15 + 0.0002 * i);
    g_sink += j::ch09::ImpliedVolNewton(call, price);
  }
}

void Dispatch() {
  j::Rng rng(1);
  const auto spots = j::ch12::GenerateTerminalSpots({}, 100'000, rng);
  g_sink += j::ch12::MeanPayOffVirtual(j::ch04::PayOffCall(100), spots);
  g_sink += j::ch12::MeanPayOffFunction(j::ch12::Call{100}, spots);
  g_sink += j::ch12::MeanPayOffVariant(j::ch12::Call{100}, spots);
  g_sink += j::ch12::MeanPayOffTemplate(j::ch12::Call{100}, spots);
}

void Factory() {
  auto& f = j::ch14::StrikePayOffFactory::Instance();
  for (int i = 0; i < 20'000; ++i) {
    g_sink += (*f.Create(i % 2 ? "call" : "put", 100))(90 + i % 20);
  }
}

// Concurrent factory lookups (shared_mutex readers) and registrations.
void FactoryThreads() {
  auto& f = j::ch14::RandomFactory::Instance();
  std::vector<std::thread> threads;
  for (int t = 0; t < 3; ++t) {
    threads.emplace_back([&f, t] {
      for (int i = 0; i < 300; ++i) {
        auto gen = f.Create("park_miller", 1, static_cast<std::uint64_t>(t + 1));
        double u[1];
        gen->GetUniforms(u);
      }
    });
  }
  threads.emplace_back([&f] {
    for (int i = 0; i < 100; ++i) {
      f.Register("tmp", [](std::size_t d, std::uint64_t s) {
        return std::make_unique<j::ch06::RandomParkMiller>(d, s);
      });
      f.Unregister("tmp");
    }
  });
  for (auto& t : threads) t.join();
  g_sink += 1.0;
}

void Pimpl() {
  j::ch16::McPricer pricer("put", 100, 1);
  pricer.SetMarket(100, 0.05, 0.01, 0.25);
  g_sink += pricer.Price(100'000, 1).price;
}

void Array() {
  j::MjArray x(256, 1.0), y(256, 2.0);
  for (int i = 0; i < 20'000; ++i) y += 0.001 * x;
  g_sink += y.Sum();
}

struct Workload {
  const char* name;
  void (*run)();
};

constexpr Workload kWorkloads[] = {
    {"mc5", Mc5},           {"mc6", Mc6},
    {"asian", Asian},       {"knockout", KnockOut},
    {"tree", Tree},         {"impliedvol", ImpliedVol},
    {"dispatch", Dispatch}, {"factory", Factory},
    {"factory_threads", FactoryThreads},
    {"pimpl", Pimpl},       {"array", Array},
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::printf("usage: joshi_profile <workload>\nworkloads:");
    for (const Workload& w : kWorkloads) std::printf(" %s", w.name);
    std::printf("\n");
    return 1;
  }
  for (const Workload& w : kWorkloads) {
    if (std::string_view(argv[1]) == w.name) {
      w.run();
      std::printf("%s: %.6f\n", w.name, g_sink);
      return 0;
    }
  }
  std::fprintf(stderr, "unknown workload %s\n", argv[1]);
  return 1;
}
