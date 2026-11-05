// Chapter 3 demo: polymorphic payoffs, deep copies, shared curves.

#include <iostream>
#include <memory>
#include <vector>

#include "ch03_polymorphism/option.h"
#include "ch03_polymorphism/payoff.h"
#include "ch03_polymorphism/yield_curve.h"

int main() {
  using namespace cpp4q::hanson::ch03;  // NOLINT(build/namespaces) demo only

  auto curve = std::make_shared<const LinearZeroCurve>(
      std::vector{0.25, 1.0, 5.0}, std::vector{0.030, 0.035, 0.040});

  std::vector<Option> book;
  book.emplace_back(std::make_unique<CallPayoff>(100.0), 1.0, curve);
  book.emplace_back(std::make_unique<PutPayoff>(95.0), 0.5, curve);
  book.emplace_back(std::make_unique<DigitalCallPayoff>(105.0, 10.0), 2.0,
                    curve);
  book.push_back(book.front());  // deep copy through Clone()

  std::cout << "curve shared by " << curve.use_count() - 1 << " options\n";
  for (const double spot : {90.0, 100.0, 110.0}) {
    std::cout << "S_T=" << spot << ':';
    for (const auto& option : book) {
      std::cout << ' ' << option.DiscountedPayoff(spot);
    }
    std::cout << '\n';
  }

  const PayoffVariant v = Put{100.0};
  std::cout << "variant put(90) = " << Evaluate(v, 90.0) << '\n';
}
