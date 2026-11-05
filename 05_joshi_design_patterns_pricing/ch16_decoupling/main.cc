// Chapter 16 demo: a client that sees only the firewall header.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ch16_decoupling/any_payoff.h"
#include "ch16_decoupling/mc_pricer.h"

int main() {
  namespace c16 = cpp4q::joshi::ch16;
  for (const std::string& name : c16::McPricer::KnownPayOffs()) {
    c16::McPricer pricer(name, 100, 1);
    pricer.SetMarket(100, 0.05, 0.0, 0.2);
    const c16::PricingResult res = pricer.Price(1 << 16, 1);
    std::printf("%-14s %.4f +- %.4f\n", name.c_str(), res.price,
                res.std_error);
  }
  // Pay-offs as plain values: a lambda, and a struct with no base class.
  struct Power {
    double strike;
    double operator()(double s) const {
      return s > strike ? (s - strike) * (s - strike) : 0.0;
    }
  };
  const std::vector<c16::AnyPayOff> book{
      [](double s) { return std::fabs(s - 100.0); }, Power{100}};
  for (const auto& p : book) std::printf("payoff(110) = %g\n", p(110));
}
