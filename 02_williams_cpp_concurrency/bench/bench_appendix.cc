// Appendix C: message-passing throughput through the order gateway.

#include <benchmark/benchmark.h>

#include <variant>

#include "appendices/order_gateway.h"

namespace {

namespace ac = cpp4q::williams::appendix_c;

// 2000 orders, half marketable (4 reports each), half resting (2 each).
void BM_GatewayOrderFlow(benchmark::State& state) {
  ac::ClientMailbox client;
  ac::OrderGateway gw(client);
  std::int64_t id = 0;
  for (auto _ : state) {
    for (int i = 0; i < 2'000; ++i) {
      ++id;
      gw.mailbox().Send(ac::NewOrder{id, 100, id % 2 ? 101.0 : 99.0});
    }
    for (int i = 0; i < 6'000; ++i) {
      benchmark::DoNotOptimize(std::get<ac::ExecutionReport>(client.Receive()));
    }
  }
  state.SetItemsProcessed(state.iterations() * 2'000);
}
BENCHMARK(BM_GatewayOrderFlow)->UseRealTime()->Unit(benchmark::kMillisecond);

}  // namespace
