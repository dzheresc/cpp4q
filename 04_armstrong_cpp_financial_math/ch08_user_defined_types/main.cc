// Chapter 8 demo: a model, two options, and put-call parity.

#include <cmath>
#include <cstdio>

#include "ch08_user_defined_types/options.h"

namespace a = cpp4q::armstrong::ch08;

int main() {
  a::BlackScholesModel model;
  model.date = 0.25;
  const a::CallOption call(105, 1.25);
  const a::PutOption put(105, 1.25);
  const double c = call.Price(model);
  const double p = put.Price(model);
  std::printf("S=%.0f vol=%.0f%% r=%.0f%% t=%.2f, K=105, T=1.25\n",
              model.stock_price, 100 * model.volatility,
              100 * model.risk_free_rate, model.date);
  std::printf("call %.6f, put %.6f\n", c, p);
  std::printf("C - P = %.12f, S - K e^{-r tau} = %.12f\n", c - p,
              model.stock_price - 105 * std::exp(-model.risk_free_rate * 1.0));
  return 0;
}
