#include <initializer_list>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch08_trees/tree.h"

namespace cpp4q::joshi::ch08 {
namespace {

constexpr double kS = 100, kK = 100, kR = 0.05, kD = 0.02, kVol = 0.2, kT = 1;

TEST(TreeTest, EuropeanConvergesToBlackScholes) {
  const TreeEuropean call(kT, ch04::PayOffCall(kK));
  const TreeEuropean put(kT, ch04::PayOffPut(kK));
  const double bs_call = BlackScholesCall(kS, kK, kR, kD, kVol, kT);
  const double bs_put = BlackScholesPut(kS, kK, kR, kD, kVol, kT);
  // First order, but not monotone: the error oscillates with where the
  // strike falls between nodes (steps * error ranges over about -2..2 for
  // this option), so the test bounds it by 3 / steps.
  for (const std::size_t steps : {50u, 200u, 800u, 1600u}) {
    SimpleBinomialTree tree(kS, kR, kD, kVol, steps, kT);
    const double bound = 3.0 / static_cast<double>(steps);
    EXPECT_NEAR(tree.GetThePrice(call), bs_call, bound) << steps;
    EXPECT_NEAR(tree.GetThePrice(put), bs_put, bound) << steps;
  }
}

TEST(TreeTest, AmericanPutCarriesEarlyExercisePremium) {
  SimpleBinomialTree tree(kS, 0.08, 0.0, kVol, 500, kT);
  const double euro = tree.GetThePrice(TreeEuropean(kT, ch04::PayOffPut(110)));
  const double amer = tree.GetThePrice(TreeAmerican(kT, ch04::PayOffPut(110)));
  EXPECT_GT(amer, euro + 0.1);
  EXPECT_GE(amer, 10.0);  // never below intrinsic
}

TEST(TreeTest, AmericanCallWithoutDividendsIsEuropean) {
  SimpleBinomialTree tree(kS, kR, 0.0, kVol, 400, kT);
  EXPECT_NEAR(tree.GetThePrice(TreeAmerican(kT, ch04::PayOffCall(kK))),
              tree.GetThePrice(TreeEuropean(kT, ch04::PayOffCall(kK))), 1e-12);
}

TEST(TreeTest, AmericanPutSelfConvergence) {
  const TreeAmerican put(kT, ch04::PayOffPut(kK));
  SimpleBinomialTree coarse(kS, kR, kD, kVol, 1000, kT);
  SimpleBinomialTree fine(kS, kR, kD, kVol, 2000, kT);
  EXPECT_NEAR(coarse.GetThePrice(put), fine.GetThePrice(put), 2e-3);
}

TEST(TreeTest, ProductsReuseOneTree) {
  SimpleBinomialTree tree(kS, kR, kD, kVol, 100, kT);
  const double a = tree.GetThePrice(TreeEuropean(kT, ch04::PayOffCall(kK)));
  tree.GetThePrice(TreeAmerican(kT, ch04::PayOffPut(kK)));
  EXPECT_EQ(tree.GetThePrice(TreeEuropean(kT, ch04::PayOffCall(kK))), a);
  EXPECT_THROW(tree.GetThePrice(TreeEuropean(2 * kT, ch04::PayOffCall(kK))),
               std::invalid_argument);
  EXPECT_THROW(SimpleBinomialTree(kS, kR, kD, kVol, 0, kT),
               std::invalid_argument);
}

// A product that only implements the per-node interface still prices
// correctly through the default layer methods.
class NodeOnlyAmericanPut final : public TreeProduct {
 public:
  NodeOnlyAmericanPut(double t, double k) : TreeProduct(t), k_(k) {}
  double FinalPayOff(double s) const override { return std::max(k_ - s, 0.0); }
  double PreFinalValue(double s, double, double future) const override {
    return std::max(std::max(k_ - s, 0.0), future);
  }
  std::unique_ptr<TreeProduct> Clone() const override {
    return std::make_unique<NodeOnlyAmericanPut>(*this);
  }

 private:
  double k_;
};

TEST(TreeTest, LayerAndNodeInterfacesAgree) {
  SimpleBinomialTree tree(kS, kR, kD, kVol, 300, kT);
  EXPECT_NEAR(tree.GetThePrice(NodeOnlyAmericanPut(kT, 105)),
              tree.GetThePrice(TreeAmerican(kT, ch04::PayOffPut(105))),
              1e-12);
}

TEST(TreeTest, TimeDependentRates) {
  // Zero vol: the tree is deterministic, a forward is priced exactly.
  const ch04::Parameters r =
      ch04::ParametersPiecewiseConstant({0.3}, {0.01, 0.07});
  SimpleBinomialTree tree(kS, r, 0.0, 0.0, 10, kT);
  const double df = std::exp(-r.Integral(0, kT));
  EXPECT_NEAR(tree.GetThePrice(TreeEuropean(kT, ch04::PayOffCall(50))),
              kS - 50 * df, 1e-10);
}

}  // namespace
}  // namespace cpp4q::joshi::ch08
