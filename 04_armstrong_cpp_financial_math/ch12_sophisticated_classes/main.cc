// Chapter 12 demo: barrier and Asian options through one pricer.

#include <cstdio>
#include <iostream>

#include "ch12_sophisticated_classes/path_dependent.h"

namespace a = cpp4q::armstrong;
using a::ch12::OptionType;

int main() {
  const a::ch10::BlackScholesModel model;
  a::ch10::Rng rng(12);
  constexpr int kSteps = 52;
  const a::ch10::MonteCarloPricer mc(100'000, kSteps);

  const a::ch12::UpAndOutCall uo(100, 130, 1);
  const a::ch12::UpAndInCall ui(100, 130, 1);
  const double shifted =
      a::ch12::DiscreteBarrierShift(130, model.volatility, 1.0 / kSteps, true);
  std::cout << "up-and-out call K=100 H=130, weekly monitoring\n"
            << "  MC:                      " << mc.Price(uo, model, rng)
            << "\n  continuous closed form:  "
            << a::ch12::UpAndOutCallPrice(model, 100, 130, 1)
            << "\n  with BGK shift (H=" << shifted << "): "
            << a::ch12::UpAndOutCallPrice(model, 100, shifted, 1) << '\n';
  std::cout << "up-and-in call MC:         " << mc.Price(ui, model, rng)
            << '\n';

  const a::ch12::GeometricAsianOption geo(OptionType::kCall, 100, 1);
  const a::ch12::ArithmeticAsianOption arith(OptionType::kCall, 100, 1);
  std::cout << "\ngeometric Asian, 52 fixings: MC " << mc.Price(geo, model, rng)
            << ", closed form "
            << a::ch12::GeometricAsianCallPrice(model, 100, 1, kSteps) << '\n'
            << "arithmetic Asian MC:         " << mc.Price(arith, model, rng)
            << '\n';
  return 0;
}
