// Chapter 6: A random numbers class.
//
// Random number generation becomes an object with an interface, so the
// generator is a pluggable strategy and decorators can be put around it:
//
//   RandomBase        draws vectors of `Dimensionality()` uniforms (one
//                     vector per path), turns them into Gaussians with the
//                     inverse cumulative normal, and can skip, reseed and
//                     reset, so runs are reproducible and splittable.
//   RandomParkMiller  the Park-Miller "minimal standard" Lehmer generator
//                     (x <- 16807 x mod 2^31 - 1).
//   AntiThetic        a decorator: every second vector is 1 - u of the
//                     previous one, so Gaussians come in +z / -z pairs.
//   RandomHalton      a low-discrepancy sequence behind the same interface
//                     (radical inverses in the first d prime bases).
//
// The book's MJArray arguments become std::span<double>.

#ifndef CPP4Q_JOSHI_CH06_RANDOM_NUMBERS_RANDOM_H_
#define CPP4Q_JOSHI_CH06_RANDOM_NUMBERS_RANDOM_H_

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "ch04_virtual_constructor/vanilla.h"
#include "ch05_strategies_statistics/statistics.h"
#include "ch05_strategies_statistics/wrapper.h"

namespace cpp4q::joshi::ch06 {

class RandomBase {
 public:
  explicit RandomBase(std::size_t dimensionality)
      : dimensionality_(dimensionality) {}
  virtual ~RandomBase() = default;

  std::size_t Dimensionality() const { return dimensionality_; }

  virtual std::unique_ptr<RandomBase> Clone() const = 0;
  // Fills `variates` (of size Dimensionality()) with uniforms in (0, 1).
  virtual void GetUniforms(std::span<double> variates) = 0;
  // Skips the next `number_of_paths` vectors.
  virtual void Skip(std::uint64_t number_of_paths) = 0;
  virtual void SetSeed(std::uint64_t seed) = 0;
  // Returns to the state just after construction or the last SetSeed.
  virtual void Reset() = 0;
  virtual void ResetDimensionality(std::size_t new_dimensionality) {
    dimensionality_ = new_dimensionality;
  }

  // Gaussians by inverting the cumulative normal, so that one uniform
  // makes one Gaussian and decorators such as AntiThetic carry over.
  virtual void GetGaussians(std::span<double> variates);

 protected:
  RandomBase(const RandomBase&) = default;
  RandomBase& operator=(const RandomBase&) = default;

 private:
  std::size_t dimensionality_;
};

// The Park-Miller minimal standard generator on {1, ..., 2^31 - 2}.
class ParkMiller {
 public:
  static constexpr std::int64_t kA = 16807;
  static constexpr std::int64_t kM = 2147483647;  // 2^31 - 1
  static constexpr std::int64_t kQ = 127773;      // kM / kA
  static constexpr std::int64_t kR = 2836;        // kM % kA

  explicit ParkMiller(std::int64_t seed = 1) { SetSeed(seed); }
  // Seeds outside [1, kM - 1] are mapped into it (0 becomes 1).
  void SetSeed(std::int64_t seed);
  std::int64_t GetOneRandomInteger();
  // Advances by n draws in O(log n): multiplies the state by a^n mod m.
  void Discard(std::uint64_t n);
  static constexpr std::int64_t Max() { return kM - 1; }
  static constexpr std::int64_t Min() { return 1; }

 private:
  std::int64_t seed_;
};

class RandomParkMiller final : public RandomBase {
 public:
  explicit RandomParkMiller(std::size_t dimensionality,
                            std::uint64_t seed = 1);
  std::unique_ptr<RandomBase> Clone() const override;
  void GetUniforms(std::span<double> variates) override;
  void Skip(std::uint64_t number_of_paths) override;
  void SetSeed(std::uint64_t seed) override;
  void Reset() override;

 private:
  ParkMiller inner_;
  std::uint64_t initial_seed_;
};

// Decorator: vectors 2k and 2k+1 are u and 1 - u.
class AntiThetic final : public RandomBase {
 public:
  explicit AntiThetic(Wrapper<RandomBase> inner);
  std::unique_ptr<RandomBase> Clone() const override;
  void GetUniforms(std::span<double> variates) override;
  void Skip(std::uint64_t number_of_paths) override;
  void SetSeed(std::uint64_t seed) override;
  void Reset() override;
  void ResetDimensionality(std::size_t new_dimensionality) override;

 private:
  Wrapper<RandomBase> inner_;
  bool odd_even_ = true;  // true: the next vector is a fresh draw
  std::vector<double> next_variates_;
};

// Halton sequence: coordinate k of point n is the radical inverse of n in
// the k-th prime base. Point 0 is skipped (it is all zeros). SetSeed(s)
// starts at point s + 1. Supports dimensionality up to 64.
class RandomHalton final : public RandomBase {
 public:
  explicit RandomHalton(std::size_t dimensionality);
  std::unique_ptr<RandomBase> Clone() const override;
  void GetUniforms(std::span<double> variates) override;
  void Skip(std::uint64_t number_of_paths) override;
  void SetSeed(std::uint64_t seed) override;
  void Reset() override;
  void ResetDimensionality(std::size_t new_dimensionality) override;

  static double RadicalInverse(std::uint64_t n, std::uint32_t base);

 private:
  std::uint64_t start_ = 1;
  std::uint64_t index_ = 1;
};

// Chapter 5's routine with the generator as a strategy (dimension 1).
void SimpleMonteCarlo6(const ch04::VanillaOption& option, double spot,
                       const ch04::Parameters& vol, const ch04::Parameters& r,
                       std::uint64_t num_paths, ch05::StatisticsMc& gatherer,
                       RandomBase& generator);

}  // namespace cpp4q::joshi::ch06

#endif  // CPP4Q_JOSHI_CH06_RANDOM_NUMBERS_RANDOM_H_
