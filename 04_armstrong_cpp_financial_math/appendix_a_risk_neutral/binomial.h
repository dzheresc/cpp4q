// Appendix A: Risk-Neutral Pricing.
//
// The appendix justifies everything the book prices: in a one-period
// binomial market a derivative is replicated exactly by delta shares and
// a bond, so its price is the cost of that portfolio, which equals the
// discounted expectation under the probability q = (e^{r dt} - d)/(u - d)
// that makes the discounted stock a martingale. Iterating gives the
// Cox-Ross-Rubinstein tree, whose prices converge to Black-Scholes.

#ifndef CPP4Q_ARMSTRONG_APPENDIX_A_RISK_NEUTRAL_BINOMIAL_H_
#define CPP4Q_ARMSTRONG_APPENDIX_A_RISK_NEUTRAL_BINOMIAL_H_

#include <functional>

namespace cpp4q::armstrong::appendix_a {

// q = (growth - d) / (u - d) with growth = exp(r dt). Throws
// std::domain_error unless d < growth < u (no arbitrage).
double RiskNeutralProbability(double up, double down, double rate, double dt);

struct Replication {
  double shares = 0.0;  // delta
  double bond = 0.0;    // cash invested at the risk-free rate
  double price = 0.0;   // shares * spot + bond
};

// The portfolio worth payoff_up / payoff_down after one step to spot*up /
// spot*down.
Replication ReplicateOneStep(double spot, double up, double down, double rate,
                             double dt, double payoff_up, double payoff_down);

enum class PayoffType { kCall, kPut };
enum class ExerciseStyle { kEuropean, kAmerican };

// CRR tree: u = exp(vol sqrt(dt)), d = 1/u.
double CrrPrice(PayoffType type, ExerciseStyle style, double strike,
                double maturity, double spot, double vol, double rate,
                int steps);

// Discounted risk-neutral expectation of any payoff of S(T) on the CRR
// tree. With payoff(s) = s it returns the spot (the martingale property)
// up to rounding.
double CrrExpectation(const std::function<double(double)>& payoff,
                      double maturity, double spot, double vol, double rate,
                      int steps);

}  // namespace cpp4q::armstrong::appendix_a

#endif  // CPP4Q_ARMSTRONG_APPENDIX_A_RISK_NEUTRAL_BINOMIAL_H_
