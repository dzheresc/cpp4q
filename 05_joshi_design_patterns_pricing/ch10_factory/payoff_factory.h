// Chapter 10: The factory.
//
// Turning a string from a user or a file into a pay-off object without a
// switch statement that lists every class. PayOffFactory is a singleton
// (one registry per program) mapping names to creation functions, and
// PayOffHelper<T> is a registrar: a global PayOffHelper<PayOffCall>
// constructed at start-up registers "call". A new pay-off class therefore
// plugs in by adding one file; neither the factory nor any caller changes.
//
// Two C++ details the book's version glosses over:
//   * The registry is a function-local static (the "Meyers singleton"), so
//     it exists before any registrar in any translation unit uses it, and
//     its initialisation is thread-safe.
//   * Registrars in a static library are dropped by the linker if nothing
//     references their object file. CMake links this chapter's library
//     with $<LINK_LIBRARY:WHOLE_ARCHIVE,...> so the registrations survive.

#ifndef CPP4Q_JOSHI_CH10_FACTORY_PAYOFF_FACTORY_H_
#define CPP4Q_JOSHI_CH10_FACTORY_PAYOFF_FACTORY_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ch04_virtual_constructor/payoff3.h"

namespace cpp4q::joshi::ch10 {

class PayOffFactory {
 public:
  using CreatePayOffFunction = std::unique_ptr<ch04::PayOff> (*)(double);

  static PayOffFactory& Instance();

  PayOffFactory(const PayOffFactory&) = delete;
  PayOffFactory& operator=(const PayOffFactory&) = delete;

  // Returns false (and keeps the existing entry) if `id` is taken.
  bool RegisterPayOff(std::string id, CreatePayOffFunction creator);
  // Throws std::out_of_range naming the known ids if `id` is unknown.
  std::unique_ptr<ch04::PayOff> CreatePayOff(std::string_view id,
                                             double strike) const;
  std::vector<std::string> RegisteredIds() const;

 private:
  PayOffFactory() = default;
  std::map<std::string, CreatePayOffFunction, std::less<>> creators_;
};

// Registers T under `id` when constructed. T must be constructible from a
// strike.
template <class T>
class PayOffHelper {
 public:
  explicit PayOffHelper(std::string id) {
    PayOffFactory::Instance().RegisterPayOff(std::move(id), &Create);
  }
  static std::unique_ptr<ch04::PayOff> Create(double strike) {
    return std::make_unique<T>(strike);
  }
};

// Registered only in payoff_registration.cc: proof that adding a product
// needs no edit to the factory. Pays |S - K|.
class PayOffStraddle final : public ch04::Cloneable<PayOffStraddle> {
 public:
  explicit PayOffStraddle(double strike) : strike_(strike) {}
  double operator()(double spot) const override;

 private:
  double strike_;
};

}  // namespace cpp4q::joshi::ch10

#endif  // CPP4Q_JOSHI_CH10_FACTORY_PAYOFF_FACTORY_H_
