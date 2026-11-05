// Chapter 3: Generating sample paths.
//
// Brownian motion on a time grid t_1 < ... < t_n (W(0) = 0), built from
// independent standard normals two ways: the random walk (increment by
// increment) and the Brownian bridge (W(t_n) first, then successive
// midpoints conditioned on their neighbours). Both give the same law; the
// bridge puts most of the path's variance into the first few normals,
// which is what makes quasi-Monte Carlo and stratification work in
// chapters 4 and 5.

#ifndef CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_BROWNIAN_H_
#define CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_BROWNIAN_H_

#include <cstddef>
#include <span>
#include <vector>

namespace cpp4q::glasserman::ch03 {

// t_i = i * maturity / steps, i = 1..steps.
std::vector<double> UniformGrid(double maturity, std::size_t steps);

// w[i] = w[i-1] + sqrt(t_i - t_{i-1}) z[i].
void RandomWalkPath(std::span<const double> times, std::span<const double> z,
                    std::span<double> w);

class BrownianBridge {
 public:
  explicit BrownianBridge(std::vector<double> times);

  std::size_t size() const { return times_.size(); }
  const std::vector<double>& times() const { return times_; }

  // z[0] determines W(t_n); later z's fill midpoints breadth-first.
  void Build(std::span<const double> z, std::span<double> w) const;

 private:
  struct Step {
    std::size_t index;
    std::size_t left;  // ignored when !has_left (left point is W(0) = 0)
    std::size_t right;
    bool has_left;
    bool has_right;
    double left_weight;
    double right_weight;
    double sd;
  };

  std::vector<double> times_;
  std::vector<Step> steps_;
};

}  // namespace cpp4q::glasserman::ch03

#endif  // CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_BROWNIAN_H_
