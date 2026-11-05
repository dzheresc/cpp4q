#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "ch02_classes/european_option.h"
#include "ch02_classes/price_buffer.h"

namespace cpp4q::hanson::ch02 {
namespace {

TEST(EuropeanOptionTest, RejectsBadInputs) {
  EXPECT_THROW(EuropeanOption(OptionType::kCall, 0.0, 1.0),
               std::invalid_argument);
  EXPECT_THROW(EuropeanOption(OptionType::kCall, 100.0, -1.0),
               std::invalid_argument);
}

TEST(EuropeanOptionTest, PricesLikeChapterOne) {
  const EuropeanOption call(OptionType::kCall, 100.0, 1.0);
  const MarketData m{.spot = 100.0, .rate = 0.05, .volatility = 0.2};
  EXPECT_NEAR(call.Price(m), 10.450583572, 1e-8);
  EXPECT_DOUBLE_EQ(call.Payoff(130.0), 30.0);
}

TEST(EuropeanOptionTest, AdvanceFloorsAtZero) {
  EuropeanOption put(OptionType::kPut, 100.0, 0.5);
  put.Advance(1.0);
  EXPECT_EQ(put.time_to_expiry(), 0.0);
}

TEST(EuropeanOptionTest, SpaceshipOrdersAndWorksAsMapKey) {
  const EuropeanOption a(OptionType::kCall, 100.0, 1.0);
  const EuropeanOption b(OptionType::kCall, 100.0, 2.0);
  const EuropeanOption c(OptionType::kPut, 50.0, 1.0);
  EXPECT_LT(a, b);
  EXPECT_LT(b, c);  // type compares first
  EXPECT_EQ(a, EuropeanOption(OptionType::kCall, 100.0, 1.0));
  std::map<EuropeanOption, int> positions{{c, 1}, {a, 2}};
  EXPECT_EQ(positions.begin()->first, a);
}

TEST(EuropeanOptionTest, StreamsReadably) {
  std::ostringstream os;
  os << EuropeanOption(OptionType::kPut, 95.5, 0.25);
  EXPECT_EQ(os.str(), "Put(K=95.50, T=0.2500)");
}

TEST(PriceBufferTest, CopyIsDeep) {
  PriceBuffer a{1.0, 2.0, 3.0};
  PriceBuffer b = a;
  b[0] = 42.0;
  EXPECT_EQ(a[0], 1.0);
  EXPECT_EQ(b.size(), 3u);
}

TEST(PriceBufferTest, MoveLeavesSourceEmpty) {
  PriceBuffer a{1.0, 2.0};
  const double* original = a.view().data();
  PriceBuffer b = std::move(a);
  EXPECT_EQ(b.view().data(), original);  // no reallocation
  EXPECT_TRUE(a.empty());                // NOLINT(bugprone-use-after-move)
  a = PriceBuffer{7.0};                  // moved-from is reusable
  EXPECT_EQ(a[0], 7.0);
}

TEST(PriceBufferTest, AssignmentCases) {
  PriceBuffer a{1.0, 2.0};
  PriceBuffer same_size{5.0, 6.0};
  PriceBuffer other_size{9.0};
  a = same_size;
  EXPECT_EQ(a, same_size);
  a = other_size;
  EXPECT_EQ(a, other_size);
  auto& self = a;
  a = self;
  EXPECT_EQ(a, other_size);
  PriceBuffer empty;
  a = empty;
  EXPECT_TRUE(a.empty());
}

TEST(PriceBufferTest, NoexceptMoveSoVectorMoves) {
  static_assert(std::is_nothrow_move_constructible_v<PriceBuffer>);
  static_assert(std::is_nothrow_move_assignable_v<PriceBuffer>);
  std::vector<PriceBuffer> v;
  v.emplace_back(PriceBuffer{1.0});
  const double* p = v[0].view().data();
  for (int i = 0; i < 64; ++i) v.emplace_back(4);
  EXPECT_EQ(v[0].view().data(), p);
}

}  // namespace
}  // namespace cpp4q::hanson::ch02
