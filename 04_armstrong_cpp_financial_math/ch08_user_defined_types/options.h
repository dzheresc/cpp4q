// Chapter 8: User-Defined Types.
//
// The book's first own types: a BlackScholesModel struct holding the
// market (a plain aggregate, public data), and CallOption / PutOption
// classes holding a contract (private data, a constructor that checks its
// arguments, const member functions). Pricing is a member function that
// takes the model by const reference.

#ifndef CPP4Q_ARMSTRONG_CH08_USER_DEFINED_TYPES_OPTIONS_H_
#define CPP4Q_ARMSTRONG_CH08_USER_DEFINED_TYPES_OPTIONS_H_

namespace cpp4q::armstrong::ch08 {

// A stock following dS = drift S dt + volatility S dW in the real world,
// and dS = r S dt + volatility S dW under the risk-neutral measure.
// `date` is the current time in years.
struct BlackScholesModel {
  double stock_price = 100.0;
  double volatility = 0.2;
  double risk_free_rate = 0.05;
  double drift = 0.1;
  double date = 0.0;
};

class CallOption {
 public:
  // Throws std::invalid_argument unless strike > 0 and maturity >= 0.
  CallOption(double strike, double maturity);

  double strike() const { return strike_; }
  double maturity() const { return maturity_; }
  double Payoff(double stock_at_maturity) const;
  // Black-Scholes price at model.date; needs model.date < maturity.
  double Price(const BlackScholesModel& model) const;

 private:
  double strike_;
  double maturity_;
};

class PutOption {
 public:
  PutOption(double strike, double maturity);

  double strike() const { return strike_; }
  double maturity() const { return maturity_; }
  double Payoff(double stock_at_maturity) const;
  double Price(const BlackScholesModel& model) const;

 private:
  double strike_;
  double maturity_;
};

}  // namespace cpp4q::armstrong::ch08

#endif  // CPP4Q_ARMSTRONG_CH08_USER_DEFINED_TYPES_OPTIONS_H_
