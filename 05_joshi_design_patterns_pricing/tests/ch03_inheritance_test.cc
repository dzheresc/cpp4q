#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch03_inheritance/payoff2.h"

namespace cpp4q::joshi::ch03 {
namespace {

// Copying through the base would slice; the base forbids it.
static_assert(!std::is_copy_constructible_v<PayOff>);
static_assert(std::has_virtual_destructor_v<PayOff>);

TEST(PayOff2Test, Values) {
  EXPECT_EQ(PayOffCall(100)(130), 30);
  EXPECT_EQ(PayOffPut(100)(130), 0);
  const PayOffDoubleDigital dd(90, 110);
  EXPECT_EQ(dd(89.9), 0);
  EXPECT_EQ(dd(90), 1);
  EXPECT_EQ(dd(110), 1);
  EXPECT_EQ(dd(110.1), 0);
}

TEST(PayOff2Test, OnePricerManyPayOffs) {
  const double s = 100, r = 0.04, vol = 0.25, t = 2;
  std::vector<std::unique_ptr<PayOff>> book;
  book.push_back(std::make_unique<PayOffCall>(110));
  book.push_back(std::make_unique<PayOffPut>(90));
  book.push_back(std::make_unique<PayOffDoubleDigital>(85, 115));
  const std::vector<double> exact{
      BlackScholesCall(s, 110, r, 0, vol, t),
      BlackScholesPut(s, 90, r, 0, vol, t),
      BlackScholesDigitalCall(s, 85, r, 0, vol, t) -
          BlackScholesDigitalCall(s, 115, r, 0, vol, t)};
  Rng rng(3);
  for (std::size_t i = 0; i < book.size(); ++i) {
    const McResult mc = SimpleMonteCarlo3(*book[i], t, s, vol, r, 100'000, rng);
    EXPECT_NEAR(mc.price, exact[i], 4 * mc.std_error) << i;
  }
}

}  // namespace
}  // namespace cpp4q::joshi::ch03
