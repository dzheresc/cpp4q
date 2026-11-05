#include "ch02_random_numbers/rng.h"

#include <cstddef>

namespace cpp4q::glasserman::ch02 {
namespace {

constexpr std::int64_t kA12 = Mrg32k3a::kA12;
constexpr std::int64_t kA13n = Mrg32k3a::kA13n;
constexpr std::int64_t kA21 = Mrg32k3a::kA21;
constexpr std::int64_t kA23n = Mrg32k3a::kA23n;

using Matrix = std::array<std::array<std::uint64_t, 3>, 3>;
__extension__ using Uint128 = unsigned __int128;  // GCC/Clang extension

std::uint64_t MulMod(std::uint64_t a, std::uint64_t b, std::uint64_t m) {
  return static_cast<std::uint64_t>((static_cast<Uint128>(a) * b) % m);
}

Matrix MatMul(const Matrix& a, const Matrix& b, std::uint64_t m) {
  Matrix c{};
  for (std::size_t i = 0; i < 3; ++i) {
    for (std::size_t j = 0; j < 3; ++j) {
      std::uint64_t sum = 0;
      for (std::size_t k = 0; k < 3; ++k) {
        sum = (sum + MulMod(a[i][k], b[k][j], m)) % m;
      }
      c[i][j] = sum;
    }
  }
  return c;
}

void MatVec(const Matrix& a, std::array<std::int64_t, 3>& v, std::uint64_t m) {
  std::array<std::int64_t, 3> out{};
  for (std::size_t i = 0; i < 3; ++i) {
    std::uint64_t sum = 0;
    for (std::size_t k = 0; k < 3; ++k) {
      sum = (sum + MulMod(a[i][k], static_cast<std::uint64_t>(v[k]), m)) % m;
    }
    out[i] = static_cast<std::int64_t>(sum);
  }
  v = out;
}

constexpr auto kUm1 = static_cast<std::uint64_t>(Mrg32k3a::kM1);
constexpr auto kUm2 = static_cast<std::uint64_t>(Mrg32k3a::kM2);

// One-step transition matrices acting on (x[n-3], x[n-2], x[n-1]).
constexpr Matrix kA1 = {{{0, 1, 0},
                         {0, 0, 1},
                         {kUm1 - static_cast<std::uint64_t>(kA13n),
                          static_cast<std::uint64_t>(kA12), 0}}};
constexpr Matrix kA2 = {{{0, 1, 0},
                         {0, 0, 1},
                         {kUm2 - static_cast<std::uint64_t>(kA23n), 0,
                          static_cast<std::uint64_t>(kA21)}}};

// powers[i] = A^(2^(base + i)), i = 0..63.
struct PowerTable {
  std::array<Matrix, 64> a1;
  std::array<Matrix, 64> a2;
};

PowerTable BuildTable(int base) {
  PowerTable t{};
  Matrix p1 = kA1;
  Matrix p2 = kA2;
  for (int i = 0; i < base; ++i) {
    p1 = MatMul(p1, p1, kUm1);
    p2 = MatMul(p2, p2, kUm2);
  }
  for (std::size_t i = 0; i < 64; ++i) {
    t.a1[i] = p1;
    t.a2[i] = p2;
    p1 = MatMul(p1, p1, kUm1);
    p2 = MatMul(p2, p2, kUm2);
  }
  return t;
}

const PowerTable& StepTable() {
  static const PowerTable table = BuildTable(0);
  return table;
}

const PowerTable& SubstreamTable() {
  static const PowerTable table = BuildTable(76);
  return table;
}

std::uint64_t SplitMix64(std::uint64_t& x) {
  std::uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

}  // namespace

Mrg32k3a::Mrg32k3a() : s1_{12345, 12345, 12345}, s2_{12345, 12345, 12345} {}

Mrg32k3a::Mrg32k3a(std::uint64_t seed) {
  std::uint64_t x = seed;
  for (auto& s : s1_) s = static_cast<std::int64_t>(SplitMix64(x) % kUm1);
  for (auto& s : s2_) s = static_cast<std::int64_t>(SplitMix64(x) % kUm2);
  // A component must not be all zero.
  if (s1_[0] == 0 && s1_[1] == 0 && s1_[2] == 0) s1_[0] = 12345;
  if (s2_[0] == 0 && s2_[1] == 0 && s2_[2] == 0) s2_[0] = 12345;
}

Mrg32k3a::result_type Mrg32k3a::operator()() {
  // Products stay below 2^53 and the compiler turns % by these constants
  // into multiplies.
  std::int64_t p1 = (kA12 * s1_[1] - kA13n * s1_[0]) % kM1;
  if (p1 < 0) p1 += kM1;
  s1_[0] = s1_[1];
  s1_[1] = s1_[2];
  s1_[2] = p1;
  std::int64_t p2 = (kA21 * s2_[2] - kA23n * s2_[0]) % kM2;
  if (p2 < 0) p2 += kM2;
  s2_[0] = s2_[1];
  s2_[1] = s2_[2];
  s2_[2] = p2;
  return static_cast<result_type>(p1 > p2 ? p1 - p2 : p1 - p2 + kM1);
}

double Mrg32k3a::NextUniform() {
  return static_cast<double>((*this)()) * kNorm;
}

void Mrg32k3a::Advance(std::uint64_t n) {
  const PowerTable& t = StepTable();
  for (std::size_t i = 0; n != 0; ++i, n >>= 1) {
    if (n & 1) {
      MatVec(t.a1[i], s1_, kUm1);
      MatVec(t.a2[i], s2_, kUm2);
    }
  }
}

void Mrg32k3a::NextSubstream() {
  const PowerTable& t = SubstreamTable();
  MatVec(t.a1[0], s1_, kUm1);
  MatVec(t.a2[0], s2_, kUm2);
}

Mrg32k3a Mrg32k3a::Substream(std::uint64_t k) const {
  Mrg32k3a copy = *this;
  const PowerTable& t = SubstreamTable();
  for (std::size_t i = 0; k != 0; ++i, k >>= 1) {
    if (k & 1) {
      MatVec(t.a1[i], copy.s1_, kUm1);
      MatVec(t.a2[i], copy.s2_, kUm2);
    }
  }
  return copy;
}

MinStd::MinStd(std::uint32_t seed) : x_(seed % 2147483647u) {
  if (x_ == 0) x_ = 1;
}

double MinStd::NextUniform() {
  x_ = static_cast<std::uint32_t>(std::uint64_t{16807} * x_ % 2147483647u);
  return x_ / 2147483647.0;
}

Randu::Randu(std::uint32_t seed) : x_(seed | 1u) {}

std::uint32_t Randu::NextInt() {
  x_ = (65539u * x_) & 0x7fffffffu;
  return x_;
}

double Randu::NextUniform() { return NextInt() / 2147483648.0; }

}  // namespace cpp4q::glasserman::ch02
