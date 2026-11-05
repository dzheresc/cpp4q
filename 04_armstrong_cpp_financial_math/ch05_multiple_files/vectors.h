// Chapter 5: Working with Multiple Files.
//
// The matlib library grows a second translation unit: vector utilities in
// the spirit of MATLAB (linspace, sum, mean, std, min, max, prctile,
// randuniform, randn). This header is the interface; vectors.cc is the
// implementation; helpers that nobody else needs stay in an anonymous
// namespace there (internal linkage), and the include guard makes the
// header safe to include twice.
//
// Random functions take the generator explicitly rather than using a
// hidden global, so results are reproducible and thread-safe.

#ifndef CPP4Q_ARMSTRONG_CH05_MULTIPLE_FILES_VECTORS_H_
#define CPP4Q_ARMSTRONG_CH05_MULTIPLE_FILES_VECTORS_H_

#include <random>
#include <span>
#include <vector>

namespace cpp4q::armstrong::ch05 {

using Rng = std::mt19937_64;

// n equally spaced points from a to b inclusive (n >= 2), or {a} if n == 1.
std::vector<double> Linspace(double a, double b, int n);

double Sum(std::span<const double> v);
// The following throw std::invalid_argument on an empty span.
double Mean(std::span<const double> v);
// Sample standard deviation (n - 1 denominator) by Welford's update;
// `population` divides by n instead. Needs size >= 2 unless population.
double StandardDeviation(std::span<const double> v, bool population = false);
double Min(std::span<const double> v);
double Max(std::span<const double> v);
// p-th percentile, p in [0, 100], MATLAB's definition: the sorted values
// sit at percentiles 100 (i - 0.5) / n, linear interpolation between them
// and the extremes outside.
double Prctile(std::span<const double> v, double p);

std::vector<double> RandUniform(int n, Rng& rng);
std::vector<double> RandN(int n, Rng& rng);

}  // namespace cpp4q::armstrong::ch05

#endif  // CPP4Q_ARMSTRONG_CH05_MULTIPLE_FILES_VECTORS_H_
