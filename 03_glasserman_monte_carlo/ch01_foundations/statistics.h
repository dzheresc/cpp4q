// Chapter 1: Foundations.
//
// The output of every Monte Carlo run in this book is an Estimate: a sample
// mean with its standard error, from which the central limit theorem gives
// a confidence interval. RunningStats accumulates it in one pass with
// Welford's update, and Merge() combines partial results (Chan et al.), so
// that parallel runs can be reduced exactly.

#ifndef CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_STATISTICS_H_
#define CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_STATISTICS_H_

#include <cmath>
#include <cstddef>

namespace cpp4q::glasserman::ch01 {

struct Estimate {
  double mean = 0.0;
  double std_error = 0.0;
  std::size_t samples = 0;

  // Half-width of the two-sided CLT interval for quantile z (1.96 = 95%).
  double HalfWidth(double z = 1.96) const { return z * std_error; }
  double Lower(double z = 1.96) const { return mean - HalfWidth(z); }
  double Upper(double z = 1.96) const { return mean + HalfWidth(z); }
  bool Contains(double x, double z = 1.96) const {
    return Lower(z) <= x && x <= Upper(z);
  }
};

class RunningStats {
 public:
  void Add(double x) {
    ++count_;
    const double delta = x - mean_;
    mean_ += delta / static_cast<double>(count_);
    m2_ += delta * (x - mean_);
  }

  void Merge(const RunningStats& other) {
    if (other.count_ == 0) return;
    if (count_ == 0) {
      *this = other;
      return;
    }
    const auto n_a = static_cast<double>(count_);
    const auto n_b = static_cast<double>(other.count_);
    const double n = n_a + n_b;
    const double delta = other.mean_ - mean_;
    mean_ += delta * n_b / n;
    m2_ += other.m2_ + delta * delta * n_a * n_b / n;
    count_ += other.count_;
  }

  std::size_t count() const { return count_; }
  double Mean() const { return mean_; }
  // Unbiased sample variance.
  double Variance() const {
    return count_ > 1 ? m2_ / static_cast<double>(count_ - 1) : 0.0;
  }
  double StdError() const {
    return count_ > 0 ? std::sqrt(Variance() / static_cast<double>(count_))
                      : 0.0;
  }
  Estimate ToEstimate() const { return {Mean(), StdError(), count_}; }

 private:
  std::size_t count_ = 0;
  double mean_ = 0.0;
  double m2_ = 0.0;
};

}  // namespace cpp4q::glasserman::ch01

#endif  // CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_STATISTICS_H_
