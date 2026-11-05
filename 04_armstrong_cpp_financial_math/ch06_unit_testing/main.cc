// Chapter 6 demo: the home-made framework testing chapter 3's matlib.

#include <cmath>
#include <iostream>

#include "ch03_functions/matlib.h"
#include "ch06_unit_testing/testing.h"

namespace {

namespace m = cpp4q::armstrong::ch03;
namespace t = cpp4q::armstrong::ch06;

const t::TestRegistrar kNormCdf("NormCdf symmetry", [] {
  for (double x : {0.1, 1.0, 2.5}) {
    t::AssertApproxEqual(m::NormCdf(x) + m::NormCdf(-x), 1.0, 1e-15);
  }
});

const t::TestRegistrar kNormInv("NormInv inverts NormCdf", [] {
  for (double p : {1e-10, 0.01, 0.3, 0.5, 0.9, 1 - 1e-10}) {
    t::AssertApproxEqual(m::NormCdf(m::NormInv(p)), p, 1e-14 + 1e-12 * p);
  }
});

const t::TestRegistrar kParity("put-call parity", [] {
  const double c = m::BlackScholesCallPrice(105, 2, 100, 0.3, 0.04);
  const double p = m::BlackScholesPutPrice(105, 2, 100, 0.3, 0.04);
  t::AssertApproxEqual(c - p, 100 - 105 * std::exp(-0.08), 1e-12);
});

const t::TestRegistrar kThrows("rejects zero vol", [] {
  t::AssertThrows([] { m::BlackScholesCallPrice(100, 1, 100, 0, 0.05); });
});

}  // namespace

int main() { return t::GlobalRegistry().RunAll(std::cout) == 0 ? 0 : 1; }
