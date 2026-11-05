// Chapter 2: Generating random numbers and random variables.
//
// Uniform generators. Mrg32k3a is L'Ecuyer's combined multiple recursive
// generator (period ~2^191), the workhorse of this book's code: besides
// being fast and statistically sound it can jump ahead by any number of
// steps in O(log n) through matrix powers, which gives every parallel
// block its own non-overlapping substream (2^76 draws apart). MinStd and
// Randu are the chapter's cautionary linear congruential generators: the
// first is merely short, the second has its triples on 15 planes.

#ifndef CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_RNG_H_
#define CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_RNG_H_

#include <array>
#include <cstdint>

namespace cpp4q::glasserman::ch02 {

class Mrg32k3a {
 public:
  using result_type = std::uint32_t;
  static constexpr std::int64_t kM1 = 4294967087;
  static constexpr std::int64_t kM2 = 4294944443;
  static constexpr std::int64_t kA12 = 1403580;
  static constexpr std::int64_t kA13n = 810728;
  static constexpr std::int64_t kA21 = 527612;
  static constexpr std::int64_t kA23n = 1370589;
  static constexpr double kNorm = 2.328306549295727688e-10;  // 1 / (kM1 + 1)

  // L'Ecuyer's reference seed: all six state words 12345.
  Mrg32k3a();
  // Derives a valid state from a 64-bit seed (SplitMix64).
  explicit Mrg32k3a(std::uint64_t seed);

  static constexpr result_type min() { return 1; }
  static constexpr result_type max() { return static_cast<result_type>(kM1); }

  // Next output in [1, kM1]: a UniformRandomBitGenerator.
  result_type operator()();
  // Next output in the open interval (0, 1). Deliberately out of line:
  // inlining it into the callers' loops measured 25-35% slower (GCC 13
  // keeps the six state words in memory across the loop; see
  // profiling/REPORT.md), while the call costs ~1 ns.
  double NextUniform();

  // Skips n outputs.
  void Advance(std::uint64_t n);
  // Skips 2^76 outputs: the start of the next substream.
  void NextSubstream();
  // A copy positioned k substreams (k * 2^76 outputs) ahead.
  Mrg32k3a Substream(std::uint64_t k) const;

  bool operator==(const Mrg32k3a&) const = default;

 private:
  std::array<std::int64_t, 3> s1_;
  std::array<std::int64_t, 3> s2_;
};

// Park-Miller "minimal standard": x <- 16807 x mod (2^31 - 1).
class MinStd {
 public:
  explicit MinStd(std::uint32_t seed = 1);
  double NextUniform();
  std::uint32_t state() const { return x_; }

 private:
  std::uint32_t x_;
};

// IBM's RANDU: x <- 65539 x mod 2^31. Every triple satisfies
// x[k+2] = 6 x[k+1] - 9 x[k] (mod 2^31). Kept as a counterexample.
class Randu {
 public:
  explicit Randu(std::uint32_t seed = 1);
  std::uint32_t NextInt();
  double NextUniform();

 private:
  std::uint32_t x_;
};

}  // namespace cpp4q::glasserman::ch02

#endif  // CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_RNG_H_
