#include "ch06_random_numbers/random.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "appendix_b_distributions/normals.h"

namespace cpp4q::joshi::ch06 {

void RandomBase::GetGaussians(std::span<double> variates) {
  GetUniforms(variates);
  for (double& x : variates) x = InverseCumulativeNormal(x);
}

void ParkMiller::SetSeed(std::int64_t seed) {
  seed_ = seed % kM;
  if (seed_ < 0) seed_ += kM;
  if (seed_ == 0) seed_ = 1;
}

std::int64_t ParkMiller::GetOneRandomInteger() {
  // The book uses Schrage's method, which computes a x mod m without
  // overflowing 32-bit integers at the cost of a division. In 64 bits the
  // product a x < 2^46 fits, and m = 2^31 - 1 is a Mersenne prime, so
  // 2^31 = 1 (mod m) and the reduction is a shift, a mask and an add.
  std::int64_t x = kA * seed_;
  x = (x & kM) + (x >> 31);
  if (x >= kM) x -= kM;
  seed_ = x;
  return seed_;
}

void ParkMiller::Discard(std::uint64_t n) {
  // Square-and-multiply; every product is below 2^62, so int64 suffices.
  std::int64_t factor = kA;
  std::int64_t result = 1;
  while (n > 0) {
    if (n & 1) result = result * factor % kM;
    factor = factor * factor % kM;
    n >>= 1;
  }
  seed_ = seed_ * result % kM;
}

RandomParkMiller::RandomParkMiller(std::size_t dimensionality,
                                   std::uint64_t seed)
    : RandomBase(dimensionality),
      inner_(static_cast<std::int64_t>(seed % ParkMiller::kM)),
      initial_seed_(seed) {}

std::unique_ptr<RandomBase> RandomParkMiller::Clone() const {
  return std::make_unique<RandomParkMiller>(*this);
}

void RandomParkMiller::GetUniforms(std::span<double> variates) {
  constexpr double kReciprocal = 1.0 / static_cast<double>(ParkMiller::kM);
  for (double& u : variates) {
    u = static_cast<double>(inner_.GetOneRandomInteger()) * kReciprocal;
  }
}

void RandomParkMiller::Skip(std::uint64_t number_of_paths) {
  // The book draws and discards number_of_paths vectors; jumping ahead is
  // exact and O(log n).
  inner_.Discard(number_of_paths * Dimensionality());
}

void RandomParkMiller::SetSeed(std::uint64_t seed) {
  initial_seed_ = seed;
  inner_.SetSeed(static_cast<std::int64_t>(seed % ParkMiller::kM));
}

void RandomParkMiller::Reset() {
  inner_.SetSeed(static_cast<std::int64_t>(initial_seed_ % ParkMiller::kM));
}

AntiThetic::AntiThetic(Wrapper<RandomBase> inner)
    : RandomBase(inner ? inner->Dimensionality() : 0),
      inner_(std::move(inner)),
      next_variates_(Dimensionality()) {
  if (!inner_) throw std::invalid_argument("AntiThetic: no inner generator");
}

std::unique_ptr<RandomBase> AntiThetic::Clone() const {
  return std::make_unique<AntiThetic>(*this);
}

void AntiThetic::GetUniforms(std::span<double> variates) {
  if (odd_even_) {
    inner_->GetUniforms(variates);
    std::copy(variates.begin(), variates.end(), next_variates_.begin());
    odd_even_ = false;
  } else {
    for (std::size_t i = 0; i < variates.size(); ++i) {
      variates[i] = 1.0 - next_variates_[i];
    }
    odd_even_ = true;
  }
}

void AntiThetic::Skip(std::uint64_t number_of_paths) {
  if (number_of_paths == 0) return;
  if (!odd_even_) {  // the pending antithetic partner is skipped first
    odd_even_ = true;
    --number_of_paths;
  }
  inner_->Skip(number_of_paths / 2);
  if (number_of_paths % 2 == 1) {
    std::vector<double> tmp(Dimensionality());
    GetUniforms(tmp);
  }
}

void AntiThetic::SetSeed(std::uint64_t seed) {
  inner_->SetSeed(seed);
  odd_even_ = true;
}

void AntiThetic::Reset() {
  inner_->Reset();
  odd_even_ = true;
}

void AntiThetic::ResetDimensionality(std::size_t new_dimensionality) {
  RandomBase::ResetDimensionality(new_dimensionality);
  next_variates_.resize(new_dimensionality);
  inner_->ResetDimensionality(new_dimensionality);
  odd_even_ = true;
}

namespace {

constexpr std::array<std::uint32_t, 64> kPrimes = {
    2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,
    43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101,
    103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167,
    173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239,
    241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311};

}  // namespace

RandomHalton::RandomHalton(std::size_t dimensionality)
    : RandomBase(dimensionality) {
  if (dimensionality > kPrimes.size()) {
    throw std::invalid_argument("RandomHalton: dimensionality above 64");
  }
}

std::unique_ptr<RandomBase> RandomHalton::Clone() const {
  return std::make_unique<RandomHalton>(*this);
}

double RandomHalton::RadicalInverse(std::uint64_t n, std::uint32_t base) {
  if (base == 2) {
    // Reversing the bits is the base-2 radical inverse.
    std::uint64_t r = n;
    r = ((r >> 1) & 0x5555555555555555ULL) | ((r & 0x5555555555555555ULL) << 1);
    r = ((r >> 2) & 0x3333333333333333ULL) | ((r & 0x3333333333333333ULL) << 2);
    r = ((r >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((r & 0x0F0F0F0F0F0F0F0FULL) << 4);
    r = ((r >> 8) & 0x00FF00FF00FF00FFULL) | ((r & 0x00FF00FF00FF00FFULL) << 8);
    r = ((r >> 16) & 0x0000FFFF0000FFFFULL) |
        ((r & 0x0000FFFF0000FFFFULL) << 16);
    r = (r >> 32) | (r << 32);
    return static_cast<double>(r >> 11) * 0x1.0p-53;
  }
  if (n <= 0xFFFFFFFFULL) {
    // Digits reversed into an integer, then one division: exact up to the
    // final rounding, and 32-bit divisions are several times cheaper than
    // 64-bit ones. base^digits <= n * base < 2^41, so nothing overflows.
    auto m = static_cast<std::uint32_t>(n);
    std::uint64_t reversed = 0;
    std::uint64_t scale = 1;
    while (m > 0) {
      const std::uint32_t q = m / base;
      reversed = reversed * base + (m - q * base);
      scale *= base;
      m = q;
    }
    return static_cast<double>(reversed) / static_cast<double>(scale);
  }
  const double inv_base = 1.0 / base;
  double inv = inv_base;
  double result = 0.0;
  while (n > 0) {
    result += static_cast<double>(n % base) * inv;
    n /= base;
    inv *= inv_base;
  }
  return result;
}

void RandomHalton::GetUniforms(std::span<double> variates) {
  for (std::size_t k = 0; k < variates.size(); ++k) {
    variates[k] = RadicalInverse(index_, kPrimes[k]);
  }
  ++index_;
}

void RandomHalton::Skip(std::uint64_t number_of_paths) {
  index_ += number_of_paths;
}

void RandomHalton::SetSeed(std::uint64_t seed) {
  start_ = seed + 1;
  index_ = start_;
}

void RandomHalton::Reset() { index_ = start_; }

void RandomHalton::ResetDimensionality(std::size_t new_dimensionality) {
  if (new_dimensionality > kPrimes.size()) {
    throw std::invalid_argument("RandomHalton: dimensionality above 64");
  }
  RandomBase::ResetDimensionality(new_dimensionality);
}

void SimpleMonteCarlo6(const ch04::VanillaOption& option, double spot,
                       const ch04::Parameters& vol, const ch04::Parameters& r,
                       std::uint64_t num_paths, ch05::StatisticsMc& gatherer,
                       RandomBase& generator) {
  generator.ResetDimensionality(1);
  const double expiry = option.expiry();
  const double variance = vol.IntegralSquare(0, expiry);
  const double root_variance = std::sqrt(variance);
  const double int_r = r.Integral(0, expiry);
  const double moved_spot = spot * std::exp(int_r - 0.5 * variance);
  const double df = std::exp(-int_r);
  double z[1];
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    generator.GetGaussians(z);
    gatherer.DumpOneResult(df *
                           option.OptionPayOff(moved_spot *
                                               std::exp(root_variance * z[0])));
  }
}

}  // namespace cpp4q::joshi::ch06
