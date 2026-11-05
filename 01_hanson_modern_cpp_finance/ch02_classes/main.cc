// Chapter 2 demo: option objects, sorting via <=>, copy vs move.

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

#include "ch02_classes/european_option.h"
#include "ch02_classes/price_buffer.h"

int main() {
  using cpp4q::hanson::ch02::EuropeanOption;
  using cpp4q::hanson::ch02::MarketData;
  using cpp4q::hanson::ch02::OptionType;
  using cpp4q::hanson::ch02::PriceBuffer;

  std::vector<EuropeanOption> book{
      {OptionType::kPut, 95.0, 0.25},
      {OptionType::kCall, 110.0, 1.0},
      {OptionType::kCall, 100.0, 0.5},
  };
  std::ranges::sort(book);  // uses the defaulted operator<=>

  const MarketData market{
      .spot = 100.0, .rate = 0.03, .dividend_yield = 0.0, .volatility = 0.25};
  for (const auto& option : book) {
    std::cout << option << " -> " << option.Price(market) << '\n';
  }

  PriceBuffer closes{101.2, 100.7, 102.3};
  PriceBuffer copy = closes;              // deep copy
  PriceBuffer moved = std::move(closes);  // pointer steal, closes now empty
  std::cout << "copy == moved: " << std::boolalpha << (copy == moved)
            << ", moved-from size: " << closes.size() << '\n';  // NOLINT

  // Appendix D: braces pick the initializer_list constructor.
  std::vector<int> two_elements{3, 7};  // {3, 7}
  std::vector<int> three_sevens(3, 7);  // {7, 7, 7}
  std::cout << "{3, 7} -> size " << two_elements.size() << ", (3, 7) -> size "
            << three_sevens.size() << '\n';
}
