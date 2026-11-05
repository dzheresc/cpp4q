// Chapter 12 demo: four dispatch mechanisms, one answer.

#include <cmath>
#include <cstdio>
#include <memory>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch12_situation_2007/modern.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c12 = j::ch12;
  const c12::BsInputs in{.spot = 100, .r = 0.05, .vol = 0.25, .expiry = 2};
  j::Rng rng(12);
  const auto spots = c12::GenerateTerminalSpots(in, 200'000, rng);
  const double df = std::exp(-in.r * in.expiry);
  const j::ch04::PayOffCall dynamic(100);
  const c12::Call value{100};
  std::printf("virtual   %.5f\n", df * c12::MeanPayOffVirtual(dynamic, spots));
  std::printf("function  %.5f\n", df * c12::MeanPayOffFunction(value, spots));
  std::printf("template  %.5f\n", df * c12::MeanPayOffTemplate(value, spots));
  std::printf("variant   %.5f\n",
              df * c12::MeanPayOffVariant(c12::PayOffVariant(value), spots));
  std::printf("exact     %.5f\n",
              j::BlackScholesCall(in.spot, 100, in.r, in.d, in.vol, in.expiry));

  const auto shared = std::make_shared<const j::ch04::PayOffCall>(100);
  const c12::SharedVanillaOption a(shared, 1), b(shared, 2);
  const c12::SharedVanillaOption c = a;
  std::printf("one pay-off shared by %ld owners\n", c.use_count());
}
