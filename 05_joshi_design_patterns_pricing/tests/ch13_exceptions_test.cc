#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch13_exceptions/exceptions.h"

namespace cpp4q::joshi::ch13 {
namespace {

static_assert(std::is_nothrow_move_constructible_v<ch04::VanillaOption>);
static_assert(std::is_nothrow_swappable_v<OptionBook>);

// A pay-off whose Clone fails once a shared budget of clones runs out.
class FragilePayOff final : public ch04::PayOff {
 public:
  FragilePayOff(double strike, std::shared_ptr<int> budget)
      : strike_(strike), budget_(std::move(budget)) {}
  double operator()(double s) const override { return s > strike_ ? 1 : 0; }
  std::unique_ptr<ch04::PayOff> Clone() const override {
    if ((*budget_)-- <= 0) throw std::bad_alloc();
    return std::make_unique<FragilePayOff>(*this);
  }

 private:
  double strike_;
  std::shared_ptr<int> budget_;
};

std::vector<ch04::VanillaOption> MakeBatch(const std::shared_ptr<int>& budget) {
  *budget = 1000;
  std::vector<ch04::VanillaOption> batch;
  for (int i = 0; i < 3; ++i) {
    batch.emplace_back(ch04::PayOffBridge(FragilePayOff(100 + i, budget)), 1);
  }
  return batch;
}

TEST(ExceptionSafetyTest, BasicGuaranteeLeavesPartialBatch) {
  auto budget = std::make_shared<int>();
  const auto batch = MakeBatch(budget);
  OptionBook book;
  book.Add(batch[0]);
  *budget = 1;  // the second clone of the batch fails
  EXPECT_THROW(book.AddAllBasic(batch), std::bad_alloc);
  EXPECT_EQ(book.size(), 2u);  // valid, but half-updated
}

TEST(ExceptionSafetyTest, StrongGuaranteeLeavesBookUnchanged) {
  auto budget = std::make_shared<int>();
  const auto batch = MakeBatch(budget);
  OptionBook book;
  book.Add(batch[0]);
  for (int allowed = 0; allowed < 4; ++allowed) {
    *budget = allowed;  // fail at every possible point of the copy
    EXPECT_THROW(book.AddAllStrong(batch), std::bad_alloc) << allowed;
    EXPECT_EQ(book.size(), 1u);
  }
  *budget = 1000;
  book.AddAllStrong(batch);
  EXPECT_EQ(book.size(), 4u);
}

TEST(ExceptionSafetyTest, ValidateNamesTheBadInput) {
  EXPECT_NO_THROW(ValidateBsInputs(100, 100, 0.2, 1));
  try {
    ValidateBsInputs(100, 100, std::nan(""), 1);
    FAIL();
  } catch (const InvalidInput& e) {
    EXPECT_NE(std::string(e.what()).find("vol"), std::string::npos);
  }
  EXPECT_THROW(ValidateBsInputs(0, 100, 0.2, 1), std::invalid_argument);
  EXPECT_THROW(ValidateBsInputs(100, 100, 0.2, -1), InvalidInput);
}

TEST(ScopeExitTest, RunsUnlessDismissed) {
  int calls = 0;
  {
    ScopeExit guard([&calls] { ++calls; });
  }
  {
    ScopeExit guard([&calls] { ++calls; });
    guard.Dismiss();
  }
  EXPECT_EQ(calls, 1);
}

TEST(ScopeExitTest, BumpRestoredOnEveryPath) {
  Market m;
  const auto bs = [](const Market& mk) {
    return BlackScholesCall(mk.spot, 100, mk.r, 0, mk.vol, 1);
  };
  EXPECT_NEAR(BumpedDelta(m, 1e-3, bs),
              BlackScholesCallDelta(100, 100, m.r, 0, m.vol, 1), 1e-7);
  EXPECT_EQ(m.spot, 100.0);
  EXPECT_THROW(BumpedDelta(m, 1.0,
                           [](const Market& mk) -> double {
                             if (mk.spot < 100) throw std::runtime_error("x");
                             return 0;
                           }),
               std::runtime_error);
  EXPECT_EQ(m.spot, 100.0);
}

TEST(CallNoThrowTest, MapsExceptionsToStatus) {
  std::string msg;
  EXPECT_EQ(CallNoThrow([] {}, &msg), Status::kOk);
  EXPECT_EQ(CallNoThrow([] { throw InvalidInput("bad"); }, &msg),
            Status::kInvalidArgument);
  EXPECT_EQ(msg, "bad");
  EXPECT_EQ(CallNoThrow([] { throw std::domain_error("d"); }, &msg),
            Status::kDomainError);
  EXPECT_EQ(CallNoThrow([] { throw std::out_of_range("o"); }, nullptr),
            Status::kOutOfRange);
  EXPECT_EQ(CallNoThrow([] { throw std::bad_alloc(); }, &msg),
            Status::kOutOfMemory);
  EXPECT_EQ(CallNoThrow([] { throw std::runtime_error("r"); }, &msg),
            Status::kRuntimeError);
  EXPECT_EQ(CallNoThrow([] { throw 42; }, &msg), Status::kUnknown);
}

}  // namespace
}  // namespace cpp4q::joshi::ch13
