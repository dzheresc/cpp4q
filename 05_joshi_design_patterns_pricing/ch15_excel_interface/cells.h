// Chapter 15: Interfacing with EXCEL (the cell model).
//
// The book exposes its pricers to Excel through xlw, which wraps the C
// API: a spreadsheet function receives cells (numbers, strings, booleans,
// blanks, errors) and ranges of cells, and must return cells and never let
// a C++ exception escape. This chapter keeps that architecture without
// Excel itself:
//
//   Cell / CellMatrix     the values a worksheet passes and receives.
//   ToDouble, ToVector    argument conversion with errors that name the
//                         argument, as xlw generates.
//   FunctionRegistry      what xlw's interface generator produces: a table
//                         of functions with names, help text and argument
//                         descriptions (the Function Wizard data), and one
//                         entry point that converts every exception into an
//                         error cell.
//
// excel_c_api.h adds the plain C ABI a real XLL (or Python's ctypes, or
// any other language) would call.

#ifndef CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_CELLS_H_
#define CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_CELLS_H_

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace cpp4q::joshi::ch15 {

// Excel's error values (the xlerr* codes of the C API).
enum class CellError {
  kNull = 0,
  kDiv0 = 7,
  kValue = 15,
  kRef = 23,
  kName = 29,
  kNum = 36,
  kNa = 42,
};

using Cell = std::variant<std::monostate, double, std::string, bool, CellError>;

class CellMatrix {
 public:
  CellMatrix() = default;
  CellMatrix(std::size_t rows, std::size_t cols)
      : rows_(rows), cols_(cols), cells_(rows * cols) {}
  // A 1x1 matrix.
  CellMatrix(Cell cell)  // NOLINT: a cell converts to a range implicitly
      : rows_(1), cols_(1), cells_{std::move(cell)} {}
  // A column of numbers.
  static CellMatrix Column(const std::vector<double>& values);
  static CellMatrix Row(const std::vector<double>& values);

  std::size_t rows() const { return rows_; }
  std::size_t cols() const { return cols_; }
  Cell& operator()(std::size_t r, std::size_t c) {
    return cells_.at(r * cols_ + c);
  }
  const Cell& operator()(std::size_t r, std::size_t c) const {
    return cells_.at(r * cols_ + c);
  }

 private:
  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::vector<Cell> cells_;
};

// Throws std::invalid_argument("<name>: ...") unless the cell holds a
// number (booleans count as 0/1, as in Excel arithmetic).
double ToDouble(const Cell& cell, std::string_view name);
std::string ToString(const Cell& cell, std::string_view name);
// A 1x1 range as a double.
double ToDouble(const CellMatrix& range, std::string_view name);
// A row or column of numbers; trailing blanks are ignored (the user
// selected more cells than were filled).
std::vector<double> ToVector(const CellMatrix& range, std::string_view name);

struct FunctionDescription {
  std::string name;
  std::string help;
  std::vector<std::string> argument_names;
  std::function<CellMatrix(const std::vector<CellMatrix>&)> body;
};

class FunctionRegistry {
 public:
  // The registry holding this chapter's pricing functions.
  static const FunctionRegistry& Pricing();

  void Add(FunctionDescription description);
  const FunctionDescription* Find(std::string_view name) const;
  std::vector<std::string> Names() const;

  // Calls `name`. Never throws: an unknown name gives #NAME?, a wrong
  // argument count or any exception gives a 1x1 string cell
  // "#ERROR: <message>" (xlw's convention, so the user sees why).
  CellMatrix Invoke(std::string_view name,
                    const std::vector<CellMatrix>& args) const noexcept;

 private:
  std::map<std::string, FunctionDescription, std::less<>> functions_;
};

}  // namespace cpp4q::joshi::ch15

#endif  // CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_CELLS_H_
