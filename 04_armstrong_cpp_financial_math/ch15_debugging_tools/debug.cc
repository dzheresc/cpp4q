#include "ch15_debugging_tools/debug.h"

#include <cmath>
#include <ostream>

namespace cpp4q::armstrong::ch15 {
namespace {

std::string_view Basename(std::string_view path) {
  const std::size_t slash = path.find_last_of('/');
  return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

constexpr std::string_view kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR"};

}  // namespace

void Require(bool condition, std::string_view message,
             std::source_location where) {
  if (condition) return;
  throw PreconditionError(std::format("{}:{}: {}: {}",
                                      Basename(where.file_name()),
                                      where.line(), where.function_name(),
                                      message));
}

double RequireFinite(double x, std::string_view what,
                     std::source_location where) {
  Require(std::isfinite(x), std::format("{} is not finite ({})", what, x),
          where);
  return x;
}

void Logger::Write(LogLevel level, std::source_location where,
                   const std::string& message) {
  *out_ << '[' << kLevelNames[static_cast<int>(level)] << "] "
        << Basename(where.file_name()) << ':' << where.line() << ' '
        << message << '\n';
}

}  // namespace cpp4q::armstrong::ch15
