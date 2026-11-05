// Chapter 15 demo: a "worksheet" calling the registered functions, and the
// C ABI.

#include <cstdio>
#include <string>
#include <variant>
#include <vector>

#include "ch15_excel_interface/cells.h"
#include "ch15_excel_interface/excel_c_api.h"

namespace {

void Print(const cpp4q::joshi::ch15::CellMatrix& m) {
  for (std::size_t r = 0; r < m.rows(); ++r) {
    for (std::size_t c = 0; c < m.cols(); ++c) {
      std::visit(
          [](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, double>) {
              std::printf(" %.6f", v);
            } else if constexpr (std::is_same_v<T, std::string>) {
              std::printf(" %s", v.c_str());
            } else {
              std::printf(" <other>");
            }
          },
          m(r, c));
    }
    std::printf("\n");
  }
}

}  // namespace

int main() {
  namespace c15 = cpp4q::joshi::ch15;
  using c15::Cell;
  using c15::CellMatrix;
  const auto& sheet = c15::FunctionRegistry::Pricing();
  for (const auto& name : sheet.Names()) {
    const auto* f = sheet.Find(name);
    std::printf("=%s(", name.c_str());
    for (std::size_t i = 0; i < f->argument_names.size(); ++i) {
      std::printf("%s%s", i ? ", " : "", f->argument_names[i].c_str());
    }
    std::printf("): %s\n", f->help.c_str());
  }
  Print(sheet.Invoke("JoshiBlackScholesCall",
                     {Cell(100.0), Cell(100.0), Cell(0.05), Cell(0.0),
                      Cell(0.2), Cell(1.0)}));
  Print(sheet.Invoke("JoshiBlackScholesCall",
                     {Cell(100.0), Cell(std::string("abc")), Cell(0.05),
                      Cell(0.0), Cell(0.2), Cell(1.0)}));
  Print(sheet.Invoke("JoshiPayOff",
                     {Cell(std::string("put")), Cell(100.0),
                      CellMatrix::Column({80, 100, 120})}));

  double price = 0;
  int status = cpp4q_joshi_bs_call(100, 100, 0.05, 0, 0.2, 1, &price);
  std::printf("C ABI: status %d price %.6f\n", status, price);
  status = cpp4q_joshi_bs_call(-1, 100, 0.05, 0, 0.2, 1, &price);
  std::printf("C ABI: status %d error '%s'\n", status,
              cpp4q_joshi_last_error());
}
