// Chapter 10 demo: consuming a named module with `import`.

#include <cstdio>

import cpp4q.hanson.pricing;

int main() {
  using cpp4q::hanson::modules::BlackScholes;
  using cpp4q::hanson::modules::OptionType;
  std::printf("module call price: %.6f\n",
              BlackScholes(OptionType::kCall, 100.0, 100.0, 0.05, 0.2, 1.0));
}
