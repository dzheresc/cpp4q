// Chapter 1: Getting Started.
//
// The first program of the book is a calculator: read a few numbers, do
// some arithmetic, print the answer. Here that arithmetic is the time value
// of money, the first formula every quant meets: what a deposit grows to
// under simple, periodic and continuous compounding, and the rate that
// makes two of them equivalent.

#ifndef CPP4Q_ARMSTRONG_CH01_GETTING_STARTED_INTEREST_H_
#define CPP4Q_ARMSTRONG_CH01_GETTING_STARTED_INTEREST_H_

namespace cpp4q::armstrong::ch01 {

// principal * (1 + rate * years).
double SimpleInterest(double principal, double rate, double years);

// principal * (1 + rate / periods_per_year)^(periods_per_year * years).
// Throws std::invalid_argument if periods_per_year < 1.
double CompoundInterest(double principal, double rate, double years,
                        int periods_per_year);

// principal * exp(rate * years): the limit of CompoundInterest as the
// number of periods grows.
double ContinuousCompounding(double principal, double rate, double years);

// The continuously compounded rate equivalent to `rate` compounded
// periods_per_year times a year: m * log(1 + rate / m).
double ContinuousEquivalentRate(double rate, int periods_per_year);

// Years for a deposit to double under continuous compounding: log(2)/rate.
double DoublingTime(double rate);

}  // namespace cpp4q::armstrong::ch01

#endif  // CPP4Q_ARMSTRONG_CH01_GETTING_STARTED_INTEREST_H_
