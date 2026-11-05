// Chapter 13: Exceptions.
//
// What the earlier designs promise when something throws, and the tools
// that make the promise cheap to keep:
//
//   * Guarantees. Every function offers at least the basic guarantee (no
//     leaks, invariants hold): ownership is always in a unique_ptr, so the
//     book's hand-written guards are not needed. OptionBook shows the gap
//     between basic and strong: AddAllBasic can leave a partial batch
//     behind, AddAllStrong (copy, modify the copy, swap) cannot.
//   * noexcept moves and swaps, which the strong guarantee relies on and
//     which let std::vector move rather than copy on reallocation.
//   * ScopeExit, a guard that undoes a temporary change (here a market
//     bump) on every exit path.
//   * A boundary that turns exceptions into status codes, for callers that
//     cannot accept exceptions (C, Excel: chapter 15).

#ifndef CPP4Q_JOSHI_CH13_EXCEPTIONS_EXCEPTIONS_H_
#define CPP4Q_JOSHI_CH13_EXCEPTIONS_EXCEPTIONS_H_

#include <exception>
#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ch04_virtual_constructor/vanilla.h"

namespace cpp4q::joshi::ch13 {

// Thrown for model inputs that make a price meaningless.
class InvalidInput : public std::invalid_argument {
 public:
  using std::invalid_argument::invalid_argument;
};

// Throws InvalidInput naming the first offending input.
void ValidateBsInputs(double spot, double strike, double vol, double expiry);

class OptionBook {
 public:
  void Add(ch04::VanillaOption option) { options_.push_back(std::move(option)); }
  // Basic guarantee: on an exception, the options added so far stay.
  void AddAllBasic(const std::vector<ch04::VanillaOption>& batch);
  // Strong guarantee: on an exception, the book is unchanged.
  void AddAllStrong(const std::vector<ch04::VanillaOption>& batch);
  std::size_t size() const { return options_.size(); }
  const std::vector<ch04::VanillaOption>& options() const { return options_; }

  friend void swap(OptionBook& a, OptionBook& b) noexcept {
    a.options_.swap(b.options_);
  }

 private:
  std::vector<ch04::VanillaOption> options_;
};

// Runs a callable when the scope exits, unless dismissed.
template <class F>
class ScopeExit {
 public:
  explicit ScopeExit(F f) noexcept(std::is_nothrow_move_constructible_v<F>)
      : f_(std::move(f)) {}
  ScopeExit(const ScopeExit&) = delete;
  ScopeExit& operator=(const ScopeExit&) = delete;
  ~ScopeExit() {
    if (active_) f_();
  }
  void Dismiss() noexcept { active_ = false; }

 private:
  F f_;
  bool active_ = true;
};

// Mutable market state, as a risk system would hold it.
struct Market {
  double spot = 100.0;
  double vol = 0.2;
  double r = 0.05;
};

// (price(spot + h) - price(spot - h)) / 2h with `market` bumped in place;
// `market` is restored on every exit, including when `price` throws.
double BumpedDelta(Market& market, double h,
                   const std::function<double(const Market&)>& price);

enum class Status {
  kOk = 0,
  kInvalidArgument = 1,
  kDomainError = 2,
  kOutOfRange = 3,
  kRuntimeError = 4,
  kOutOfMemory = 5,
  kUnknown = 6,
};

// Runs `f`, catching everything. Returns the status and, on failure, writes
// the message to `message` (if non-null). Never throws.
template <class F>
Status CallNoThrow(F&& f, std::string* message) noexcept {
  const auto fail = [message](Status s, const char* what) noexcept {
    if (message != nullptr) {
      try {
        *message = what;
      } catch (...) {  // allocation failure while reporting: drop the text
      }
    }
    return s;
  };
  try {
    std::forward<F>(f)();
    return Status::kOk;
  } catch (const std::invalid_argument& e) {
    return fail(Status::kInvalidArgument, e.what());
  } catch (const std::domain_error& e) {
    return fail(Status::kDomainError, e.what());
  } catch (const std::out_of_range& e) {
    return fail(Status::kOutOfRange, e.what());
  } catch (const std::bad_alloc& e) {
    return fail(Status::kOutOfMemory, e.what());
  } catch (const std::exception& e) {
    return fail(Status::kRuntimeError, e.what());
  } catch (...) {
    return fail(Status::kUnknown, "unknown exception");
  }
}

}  // namespace cpp4q::joshi::ch13

#endif  // CPP4Q_JOSHI_CH13_EXCEPTIONS_EXCEPTIONS_H_
