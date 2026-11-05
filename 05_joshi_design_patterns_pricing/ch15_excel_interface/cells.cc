#include "ch15_excel_interface/cells.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch05_strategies_statistics/statistics.h"
#include "ch06_random_numbers/random.h"
#include "ch07_exotics_engine/exotic_engine.h"
#include "ch09_solvers_implied_vol/solvers.h"
#include "ch13_exceptions/exceptions.h"
#include "ch14_template_factory/registrations.h"

namespace cpp4q::joshi::ch15 {
namespace {

[[noreturn]] void BadArgument(std::string_view name, std::string_view what) {
  throw std::invalid_argument(std::string(name) + ": " + std::string(what));
}

}  // namespace

CellMatrix CellMatrix::Column(const std::vector<double>& values) {
  CellMatrix m(values.size(), 1);
  for (std::size_t i = 0; i < values.size(); ++i) m(i, 0) = values[i];
  return m;
}

CellMatrix CellMatrix::Row(const std::vector<double>& values) {
  CellMatrix m(1, values.size());
  for (std::size_t i = 0; i < values.size(); ++i) m(0, i) = values[i];
  return m;
}

double ToDouble(const Cell& cell, std::string_view name) {
  if (const double* d = std::get_if<double>(&cell)) return *d;
  if (const bool* b = std::get_if<bool>(&cell)) return *b ? 1.0 : 0.0;
  if (std::holds_alternative<std::monostate>(cell)) BadArgument(name, "is empty");
  if (std::holds_alternative<CellError>(cell)) BadArgument(name, "is an error");
  BadArgument(name, "is not a number");
}

std::string ToString(const Cell& cell, std::string_view name) {
  if (const std::string* s = std::get_if<std::string>(&cell)) return *s;
  BadArgument(name, "is not a string");
}

double ToDouble(const CellMatrix& range, std::string_view name) {
  if (range.rows() != 1 || range.cols() != 1) {
    BadArgument(name, "must be a single cell");
  }
  return ToDouble(range(0, 0), name);
}

std::vector<double> ToVector(const CellMatrix& range, std::string_view name) {
  if (range.rows() != 1 && range.cols() != 1) {
    BadArgument(name, "must be a row or a column");
  }
  const std::size_t n = range.rows() * range.cols();
  std::size_t last = n;
  const auto at = [&](std::size_t i) -> const Cell& {
    return range.rows() == 1 ? range(0, i) : range(i, 0);
  };
  while (last > 0 && std::holds_alternative<std::monostate>(at(last - 1))) {
    --last;
  }
  std::vector<double> out;
  out.reserve(last);
  for (std::size_t i = 0; i < last; ++i) out.push_back(ToDouble(at(i), name));
  return out;
}

void FunctionRegistry::Add(FunctionDescription description) {
  std::string key = description.name;
  functions_.insert_or_assign(std::move(key), std::move(description));
}

const FunctionDescription* FunctionRegistry::Find(std::string_view name) const {
  const auto it = functions_.find(name);
  return it == functions_.end() ? nullptr : &it->second;
}

std::vector<std::string> FunctionRegistry::Names() const {
  std::vector<std::string> names;
  for (const auto& [name, f] : functions_) names.push_back(name);
  return names;
}

CellMatrix FunctionRegistry::Invoke(
    std::string_view name, const std::vector<CellMatrix>& args) const noexcept {
  const FunctionDescription* f = Find(name);
  if (f == nullptr) return CellMatrix(Cell(CellError::kName));
  CellMatrix result;
  std::string message;
  const ch13::Status status = ch13::CallNoThrow(
      [&] {
        if (args.size() != f->argument_names.size()) {
          throw std::invalid_argument(
              "expected " + std::to_string(f->argument_names.size()) +
              " arguments, got " + std::to_string(args.size()));
        }
        result = f->body(args);
      },
      &message);
  if (status == ch13::Status::kOk) return result;
  try {
    return CellMatrix(Cell("#ERROR: " + message));
  } catch (...) {
    return CellMatrix(Cell(CellError::kValue));
  }
}

const FunctionRegistry& FunctionRegistry::Pricing() {
  static const FunctionRegistry registry = [] {
    FunctionRegistry r;
    r.Add({"JoshiBlackScholesCall",
           "Black-Scholes price of a European call",
           {"Spot", "Strike", "Rate", "Dividend", "Vol", "Expiry"},
           [](const std::vector<CellMatrix>& a) -> CellMatrix {
             const double spot = ToDouble(a[0], "Spot");
             const double strike = ToDouble(a[1], "Strike");
             const double vol = ToDouble(a[4], "Vol");
             const double expiry = ToDouble(a[5], "Expiry");
             ch13::ValidateBsInputs(spot, strike, vol, expiry);
             return Cell(BlackScholesCall(spot, strike, ToDouble(a[2], "Rate"),
                                          ToDouble(a[3], "Dividend"), vol,
                                          expiry));
           }});
    r.Add({"JoshiImpliedVol",
           "Implied volatility of a European call price",
           {"Price", "Spot", "Strike", "Rate", "Dividend", "Expiry"},
           [](const std::vector<CellMatrix>& a) -> CellMatrix {
             const ch09::BsCall call(
                 ToDouble(a[3], "Rate"), ToDouble(a[4], "Dividend"),
                 ToDouble(a[5], "Expiry"), ToDouble(a[1], "Spot"),
                 ToDouble(a[2], "Strike"));
             return Cell(ch09::ImpliedVolNewton(call, ToDouble(a[0], "Price")));
           }});
    r.Add({"JoshiPayOff",
           "Pay-off by name (call, put, digital_call) at each spot",
           {"Name", "Strike", "Spots"},
           [](const std::vector<CellMatrix>& a) -> CellMatrix {
             if (a[0].rows() != 1 || a[0].cols() != 1) {
               BadArgument("Name", "must be a single cell");
             }
             const auto payoff = ch14::StrikePayOffFactory::Instance().Create(
                 ToString(a[0](0, 0), "Name"), ToDouble(a[1], "Strike"));
             std::vector<double> values = ToVector(a[2], "Spots");
             for (double& v : values) v = (*payoff)(v);
             return CellMatrix::Column(values);
           }});
    r.Add({"JoshiAsianCallMc",
           "Arithmetic Asian call by Monte Carlo: {price, standard error}",
           {"Spot", "Strike", "Rate", "Dividend", "Vol", "FixingTimes",
            "Paths", "Seed"},
           [](const std::vector<CellMatrix>& a) -> CellMatrix {
             const std::vector<double> times = ToVector(a[5], "FixingTimes");
             const double paths = ToDouble(a[6], "Paths");
             const double seed = ToDouble(a[7], "Seed");
             if (!(paths >= 2 && paths <= 1e9)) {
               BadArgument("Paths", "must be between 2 and 1e9");
             }
             if (times.empty()) BadArgument("FixingTimes", "is empty");
             const ch07::PathDependentAsian asian(
                 times, times.back(),
                 ch04::PayOffCall(ToDouble(a[1], "Strike")));
             ch07::ExoticBsEngine engine(
                 asian, ToDouble(a[2], "Rate"), ToDouble(a[3], "Dividend"),
                 ToDouble(a[4], "Vol"),
                 ch06::RandomParkMiller(times.size(),
                                        static_cast<std::uint64_t>(
                                            std::fabs(seed))),
                 ToDouble(a[0], "Spot"));
             ch05::StatisticsMoments stats;
             engine.DoSimulation(stats, static_cast<std::uint64_t>(paths));
             const auto row = stats.GetResultsSoFar()[0];
             return CellMatrix::Row({row[0], row[1]});
           }});
    return r;
  }();
  return registry;
}

}  // namespace cpp4q::joshi::ch15
