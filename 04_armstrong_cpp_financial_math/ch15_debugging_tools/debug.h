// Chapter 15: Debugging and Development Tools.
//
// The chapter is about tools outside the code: debuggers, profilers and
// memory checkers. Those are applied to this whole book in profiling/
// (valgrind memcheck, callgrind, cachegrind, massif; AddressSanitizer,
// UndefinedBehaviorSanitizer, ThreadSanitizer). What belongs in the code
// is the support those tools need: precondition checks that say where
// they failed, levelled logging that costs nothing when switched off, and
// a scoped timer for quick measurements before reaching for a profiler.

#ifndef CPP4Q_ARMSTRONG_CH15_DEBUGGING_TOOLS_DEBUG_H_
#define CPP4Q_ARMSTRONG_CH15_DEBUGGING_TOOLS_DEBUG_H_

#include <chrono>
#include <format>
#include <functional>
#include <iosfwd>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace cpp4q::armstrong::ch15 {

// Thrown by Require; what() reads "file:line: function: message".
class PreconditionError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

void Require(bool condition, std::string_view message,
             std::source_location where = std::source_location::current());

// Throws PreconditionError unless x is finite (not NaN or inf): the check
// to put on a pricer's output when hunting the first NaN.
double RequireFinite(double x, std::string_view what,
                     std::source_location where =
                         std::source_location::current());

enum class LogLevel { kDebug = 0, kInfo = 1, kWarning = 2, kError = 3 };

// Writes "[LEVEL] file:line message" lines at or above a threshold.
// Formatting happens only for messages that pass the threshold.
class Logger {
 public:
  explicit Logger(std::ostream& out, LogLevel threshold = LogLevel::kInfo)
      : out_(&out), threshold_(threshold) {}
  void set_threshold(LogLevel level) { threshold_ = level; }
  bool Enabled(LogLevel level) const { return level >= threshold_; }

  template <typename... Args>
  void Log(LogLevel level, std::source_location where,
           std::format_string<Args...> fmt, Args&&... args) {
    if (!Enabled(level)) return;
    Write(level, where, std::format(fmt, std::forward<Args>(args)...));
  }

 private:
  void Write(LogLevel level, std::source_location where,
             const std::string& message);
  std::ostream* out_;
  LogLevel threshold_;
};

// Calls `report(elapsed)` when it goes out of scope.
class ScopedTimer {
 public:
  using Clock = std::chrono::steady_clock;
  explicit ScopedTimer(std::function<void(Clock::duration)> report)
      : report_(std::move(report)), start_(Clock::now()) {}
  ScopedTimer(const ScopedTimer&) = delete;
  ScopedTimer& operator=(const ScopedTimer&) = delete;
  ~ScopedTimer() { report_(Clock::now() - start_); }

 private:
  std::function<void(Clock::duration)> report_;
  Clock::time_point start_;
};

}  // namespace cpp4q::armstrong::ch15

#endif  // CPP4Q_ARMSTRONG_CH15_DEBUGGING_TOOLS_DEBUG_H_
