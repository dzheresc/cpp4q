// Chapter 10 demo: one generic tree pricer, many payoff types.

#include <format>
#include <iostream>
#include <memory>
#include <vector>

#include "ch03_polymorphism/payoff.h"
#include "ch03_polymorphism/yield_curve.h"
#include "ch10_modules_concepts/concepts.h"
#include "ch10_modules_concepts/generic_pricing.h"

namespace {

struct Flow {
  double time;
  double amount;
};

}  // namespace

int main() {
  namespace ch03 = cpp4q::hanson::ch03;
  namespace ch10 = cpp4q::hanson::ch10;

  static_assert(ch10::PayoffFunction<ch03::CallPayoff>);
  static_assert(ch10::PayoffFunction<ch03::Put>);
  static_assert(ch10::DiscountCurve<ch03::FlatYieldCurve>);
  static_assert(!ch10::DiscountCurve<double>);

  const ch10::TreeParams<double> p{.spot = 100.0,
                                   .rate = 0.05,
                                   .dividend_yield = 0.0,
                                   .volatility = 0.2,
                                   .expiry = 1.0,
                                   .steps = 1000};
  const ch03::PutPayoff put(100.0);  // virtual hierarchy
  const auto lambda_put = [](double s) { return s < 100.0 ? 100.0 - s : 0.0; };

  std::cout << std::format(
      "European put {:.6f}  American put {:.6f}\n",
      ch10::PriceBinomial(p, put, ch10::ExerciseStyle::kEuropean),
      ch10::PriceBinomial(p, lambda_put, ch10::ExerciseStyle::kAmerican));

  // Same algorithm instantiated for float.
  const ch10::TreeParams<float> pf{100.0f, 0.05f, 0.0f, 0.2f, 1.0f, 200};
  std::cout << std::format("float tree call {:.4f}\n",
                           ch10::PriceBinomial(pf, ch03::Call{100.0f},
                                               ch10::ExerciseStyle::kEuropean));

  const std::vector<Flow> bond{
      {0.5, 2.0}, {1.0, 2.0}, {1.5, 2.0}, {2.0, 102.0}};
  std::cout << std::format("NPV on flat 4% curve: {:.6f}\n",
                           ch10::Npv(bond, ch03::FlatYieldCurve(0.04)));
}
