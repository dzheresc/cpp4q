// Chapter 11 demo: raw arrays and C strings, then a CSV of prices to
// historical volatility.

#include <cstdio>
#include <sstream>
#include <vector>

#include "ch11_arrays_strings_pointers/arrays.h"

namespace a = cpp4q::armstrong::ch11;

int main() {
  double raw[] = {1.0, 2.0, 3.0, 4.0, 5.0};
  a::ReverseInPlace(raw, raw + 5);
  std::printf("reversed: %g %g %g %g %g, sum %g\n", raw[0], raw[1], raw[2],
              raw[3], raw[4], a::SumArray(raw, 5));
  char small[8];
  const std::size_t needed = a::CopyCString(small, sizeof small, "volatility");
  std::printf("copied \"%s\" (needed %zu chars, truncated: %s)\n", small,
              needed, needed >= sizeof small ? "yes" : "no");

  std::istringstream csv(
      "date,close\n2024-01-02,100.0\n2024-01-03,101.5\n2024-01-04,100.7\n"
      "2024-01-05,102.3\n2024-01-08,101.9\n2024-01-09,103.4\n");
  const auto records = a::ReadPriceCsv(csv);
  std::vector<double> closes;
  for (const auto& r : records) closes.push_back(r.close);
  std::printf("%zu prices from %s to %s, annualised vol %.2f%%\n",
              records.size(), records.front().date.c_str(),
              records.back().date.c_str(),
              100 * a::HistoricalVolatility(closes));
  return 0;
}
