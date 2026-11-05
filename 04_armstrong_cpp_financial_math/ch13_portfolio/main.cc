// Chapter 13 demo: a straddle, a bull spread and the put-call parity
// portfolio, with their Greeks.

#include <cstdio>
#include <memory>

#include "ch13_portfolio/portfolio.h"

namespace a = cpp4q::armstrong;

namespace {

void Report(const char* name, const a::ch13::Portfolio& p,
            const a::ch13::BlackScholesModel& m) {
  std::printf("%-18s price %10.6f  delta %9.6f  gamma %9.6f  vega %9.5f\n",
              name, p.Price(m), p.Delta(m), p.Gamma(m), p.Vega(m));
}

}  // namespace

int main() {
  const a::ch13::BlackScholesModel model;
  a::ch13::Portfolio straddle;
  straddle.Add(1, std::make_shared<a::ch10::EuropeanCall>(100, 1));
  straddle.Add(1, std::make_shared<a::ch10::EuropeanPut>(100, 1));
  a::ch13::Portfolio bull;
  bull.Add(1, std::make_shared<a::ch10::EuropeanCall>(95, 1));
  bull.Add(-1, std::make_shared<a::ch10::EuropeanCall>(105, 1));
  Report("straddle", straddle, model);
  Report("bull call spread", bull, model);
  Report("put-call parity", a::ch13::PutCallParityPortfolio(100, 1), model);
  return 0;
}
