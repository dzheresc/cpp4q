// Chapter 21: Next Steps.
//
// The book closes by pointing beyond Monte Carlo; the natural next method
// is the Black-Scholes PDE solved by finite differences. This is the
// theta scheme on a uniform grid in S with Crank-Nicolson time stepping
// (theta = 1/2) after a few fully implicit Rannacher steps that damp the
// payoff kink, a tridiagonal (Thomas) solve for European exercise and
// projected SOR for American exercise. The grid is stretched slightly so
// the spot is a node (price, delta and gamma are read off it directly),
// and the initial condition is the cell-averaged payoff, which makes the
// error independent of where the strike falls between nodes.

#ifndef CPP4Q_ARMSTRONG_CH21_NEXT_STEPS_PDE_H_
#define CPP4Q_ARMSTRONG_CH21_NEXT_STEPS_PDE_H_

#include <span>
#include <vector>

namespace cpp4q::armstrong::ch21 {

enum class PayoffType { kCall, kPut };
enum class ExerciseStyle { kEuropean, kAmerican };

struct PdeSettings {
  int n_space = 400;          // intervals in S over [0, s_max]
  int n_time = 400;           // time steps
  double s_max_multiple = 4;  // s_max ~ multiple * max(spot, strike)
  int rannacher_steps = 2;    // initial fully implicit steps
  double sor_omega = 1.2;     // over-relaxation for American exercise
  double sor_tolerance = 1e-10;
};

struct PdeResult {
  double price = 0.0;
  double delta = 0.0;
  double gamma = 0.0;
};

// Throws std::invalid_argument on non-positive inputs or a grid too
// coarse to contain the spot away from the boundary.
PdeResult PricePde(PayoffType type, ExerciseStyle style, double strike,
                   double maturity, double spot, double vol, double rate,
                   const PdeSettings& settings = {});

// Solves the tridiagonal system with sub-diagonal a, diagonal b and
// super-diagonal c (a[0] and c[n-1] unused) in place of d. Exposed for
// testing; needs a diagonally dominant system.
void SolveTridiagonal(std::span<const double> a, std::span<const double> b,
                      std::span<const double> c, std::span<double> d);

}  // namespace cpp4q::armstrong::ch21

#endif  // CPP4Q_ARMSTRONG_CH21_NEXT_STEPS_PDE_H_
