// Chapter 16: Decoupling.
//
// Every header a client includes is a compile-time dependency: change it
// and the client rebuilds; include its includes and the client depends on
// those too. This header is a compilation firewall. It includes only
// standard headers, forward-declares the pay-off it can accept, and hides
// the factories, generators, statistics gatherers and their headers
// behind a pointer to an implementation (pimpl). Clients of McPricer
// never recompile when any of chapters 4-14 changes, and the test that
// uses it includes nothing else.

#ifndef CPP4Q_JOSHI_CH16_DECOUPLING_MC_PRICER_H_
#define CPP4Q_JOSHI_CH16_DECOUPLING_MC_PRICER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cpp4q::joshi {
namespace ch04 {
class PayOff;  // a forward declaration is all a reference parameter needs
}  // namespace ch04

namespace ch16 {

struct PricingResult {
  double price;
  double std_error;
};

class McPricer {
 public:
  // `payoff_name` is looked up in the chapter 14 pay-off factory; throws
  // std::out_of_range if unknown.
  McPricer(const std::string& payoff_name, double strike, double expiry);
  // Uses a clone of `payoff`.
  McPricer(const ch04::PayOff& payoff, double expiry);
  ~McPricer();
  McPricer(const McPricer& other);
  McPricer& operator=(const McPricer& other);
  McPricer(McPricer&& other) noexcept;
  McPricer& operator=(McPricer&& other) noexcept;

  void SetMarket(double spot, double r, double d, double vol);
  // "park_miller", "mt19937_64", "halton", ... (chapter 14 names).
  void SetGenerator(const std::string& generator_name);
  // Returns the discounted mean pay-off and its standard error.
  PricingResult Price(std::uint64_t num_paths, std::uint64_t seed) const;
  static std::vector<std::string> KnownPayOffs();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ch16
}  // namespace cpp4q::joshi

#endif  // CPP4Q_JOSHI_CH16_DECOUPLING_MC_PRICER_H_
