// Chapter 10 demo: one pricer, many options, three pricing methods.

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ch10_interfaces/interfaces.h"

namespace a = cpp4q::armstrong::ch10;

int main() {
  const a::BlackScholesModel model;
  std::vector<std::pair<std::string, std::unique_ptr<a::PathIndependentOption>>>
      book;
  book.emplace_back("call 100", std::make_unique<a::EuropeanCall>(100, 1));
  book.emplace_back("put 100", std::make_unique<a::EuropeanPut>(100, 1));
  book.emplace_back("digital call 110",
                    std::make_unique<a::DigitalCall>(110, 1));
  book.emplace_back("digital put 90", std::make_unique<a::DigitalPut>(90, 1));
  a::Rng rng(10);
  const a::MonteCarloPricer mc(200'000);
  std::printf("%-18s %12s %12s %12s %10s\n", "option", "closed form",
              "quadrature", "MC", "std err");
  for (const auto& [name, option] : book) {
    const auto& priceable = dynamic_cast<const a::Priceable&>(*option);
    const auto e = mc.Price(*option, model, rng);
    std::printf("%-18s %12.6f %12.6f %12.6f %10.6f\n", name.c_str(),
                priceable.Price(model),
                a::PriceByIntegration(*option, model, 2000), e.price,
                e.std_error);
  }
  return 0;
}
