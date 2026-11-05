#include "ch11_arrays_strings_pointers/arrays.h"

#include <charconv>
#include <cmath>
#include <istream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace cpp4q::armstrong::ch11 {

double SumArray(const double* values, std::size_t n) {
  double sum = 0.0;
  for (const double* p = values; p != values + n; ++p) sum += *p;
  return sum;
}

void ReverseInPlace(double* first, double* last) {
  while (first < last) {
    --last;
    if (first == last) break;
    std::swap(*first, *last);
    ++first;
  }
}

std::size_t CStringLength(const char* s) {
  const char* p = s;
  while (*p != '\0') ++p;
  return static_cast<std::size_t>(p - s);
}

std::size_t CopyCString(char* dest, std::size_t capacity, const char* src) {
  const std::size_t len = CStringLength(src);
  if (capacity == 0) return len;
  const std::size_t n = len < capacity - 1 ? len : capacity - 1;
  for (std::size_t i = 0; i < n; ++i) dest[i] = src[i];
  dest[n] = '\0';
  return len;
}

std::vector<std::string_view> Split(std::string_view text, char delimiter) {
  std::vector<std::string_view> fields;
  std::size_t start = 0;
  while (true) {
    const std::size_t end = text.find(delimiter, start);
    if (end == std::string_view::npos) {
      fields.push_back(text.substr(start));
      return fields;
    }
    fields.push_back(text.substr(start, end - start));
    start = end + 1;
  }
}

std::string_view Trim(std::string_view text) {
  constexpr std::string_view kSpace = " \t\r";
  const std::size_t first = text.find_first_not_of(kSpace);
  if (first == std::string_view::npos) return {};
  const std::size_t last = text.find_last_not_of(kSpace);
  return text.substr(first, last - first + 1);
}

std::vector<PriceRecord> ReadPriceCsv(std::istream& in) {
  std::vector<PriceRecord> records;
  std::string line;
  int line_no = 0;
  bool header = true;
  while (std::getline(in, line)) {
    ++line_no;
    if (Trim(line).empty()) continue;
    if (header) {
      header = false;
      continue;
    }
    const auto fields = Split(line, ',');
    if (fields.size() != 2) {
      throw std::runtime_error("line " + std::to_string(line_no) +
                               ": expected 2 fields");
    }
    const std::string_view value = Trim(fields[1]);
    double close = 0.0;
    const auto [ptr, ec] =
        std::from_chars(value.data(), value.data() + value.size(), close);
    if (ec != std::errc() || ptr != value.data() + value.size()) {
      throw std::runtime_error("line " + std::to_string(line_no) +
                               ": bad price '" + std::string(value) + "'");
    }
    records.push_back({std::string(Trim(fields[0])), close});
  }
  return records;
}

std::vector<double> LogReturns(std::span<const double> prices) {
  if (prices.size() < 2) throw std::invalid_argument("need >= 2 prices");
  std::vector<double> r;
  r.reserve(prices.size() - 1);
  for (std::size_t i = 1; i < prices.size(); ++i) {
    if (!(prices[i - 1] > 0 && prices[i] > 0)) {
      throw std::invalid_argument("prices must be positive");
    }
    r.push_back(std::log(prices[i] / prices[i - 1]));
  }
  return r;
}

double HistoricalVolatility(std::span<const double> prices,
                            double periods_per_year) {
  const std::vector<double> r = LogReturns(prices);
  if (r.size() < 2) throw std::invalid_argument("need >= 3 prices");
  double mean = 0.0;
  for (double x : r) mean += x;
  mean /= static_cast<double>(r.size());
  double ss = 0.0;
  for (double x : r) ss += (x - mean) * (x - mean);
  return std::sqrt(ss / static_cast<double>(r.size() - 1) * periods_per_year);
}

}  // namespace cpp4q::armstrong::ch11
