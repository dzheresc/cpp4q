// Chapter 1: An Overview of C++.
//
// Free functions, enum class, constexpr, std::numbers and value types,
// illustrated with closed-form Black-Scholes-Merton pricing.

#ifndef CPP4Q_HANSON_CH01_OVERVIEW_BLACK_SCHOLES_H_
#define CPP4Q_HANSON_CH01_OVERVIEW_BLACK_SCHOLES_H_

#include <numbers>

namespace cpp4q::hanson::ch01 {

enum class OptionType { kCall, kPut };

// Market and contract inputs. Rates and yields are continuously compounded,
// time is in years.
struct BlackScholesInputs {
  double spot = 0.0;
  double strike = 0.0;
  double rate = 0.0;
  double dividend_yield = 0.0;
  double volatility = 0.0;
  double time_to_expiry = 0.0;
};

struct Greeks {
  double delta = 0.0;
  double gamma = 0.0;
  double vega = 0.0;   // per unit of volatility (not per 1%)
  double theta = 0.0;  // per year
  double rho = 0.0;    // per unit of rate
};

inline constexpr double kInvSqrt2Pi =
    std::numbers::inv_sqrtpi / std::numbers::sqrt2;

// Standard normal density and cumulative distribution.
double NormPdf(double x);
double NormCdf(double x);

// Discount factor exp(-r t).
double DiscountFactor(double rate, double time);

// Discretely compounded growth (1 + r/m)^(m t). constexpr: m*t must be a
// whole number of periods; usable at compile time.
constexpr double CompoundGrowth(double rate, int periods_per_year, int years) {
  const double per_period = 1.0 + rate / periods_per_year;
  double growth = 1.0;
  for (int i = 0; i < periods_per_year * years; ++i) growth *= per_period;
  return growth;
}

// Undiscounted payoff at expiry.
constexpr double Payoff(OptionType type, double spot, double strike) {
  const double intrinsic =
      type == OptionType::kCall ? spot - strike : strike - spot;
  return intrinsic > 0.0 ? intrinsic : 0.0;
}

// Black-Scholes-Merton price. Degenerate inputs (zero vol or zero time) fall
// back to the discounted forward intrinsic value.
double BlackScholesPrice(OptionType type, const BlackScholesInputs& in);

Greeks BlackScholesGreeks(OptionType type, const BlackScholesInputs& in);

}  // namespace cpp4q::hanson::ch01

#endif  // CPP4Q_HANSON_CH01_OVERVIEW_BLACK_SCHOLES_H_
