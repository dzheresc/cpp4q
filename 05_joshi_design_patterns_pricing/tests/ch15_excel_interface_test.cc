#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <variant>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch15_excel_interface/cells.h"
#include "ch15_excel_interface/excel_c_api.h"

namespace cpp4q::joshi::ch15 {
namespace {

// Returns the text of a 1x1 string result, "" otherwise. (Returns a copy:
// the matrix is usually a temporary.)
std::string ErrorText(const CellMatrix& m) {
  const auto* s = std::get_if<std::string>(&m(0, 0));
  return s ? *s : std::string();
}

TEST(CellsTest, Conversions) {
  EXPECT_EQ(ToDouble(Cell(2.5), "x"), 2.5);
  EXPECT_EQ(ToDouble(Cell(true), "x"), 1.0);
  EXPECT_THROW(ToDouble(Cell(), "x"), std::invalid_argument);
  EXPECT_THROW(ToDouble(Cell(CellError::kNa), "x"), std::invalid_argument);
  try {
    ToDouble(Cell(std::string("abc")), "Strike");
    FAIL();
  } catch (const std::invalid_argument& e) {
    EXPECT_EQ(std::string(e.what()), "Strike: is not a number");
  }
  CellMatrix col(4, 1);
  col(0, 0) = 1.0;
  col(1, 0) = 2.0;  // two trailing blanks: the user over-selected
  EXPECT_EQ(ToVector(col, "v"), (std::vector<double>{1, 2}));
  col(3, 0) = 4.0;  // a blank in the middle is an error
  EXPECT_THROW(ToVector(col, "v"), std::invalid_argument);
  EXPECT_THROW(ToVector(CellMatrix(2, 2), "v"), std::invalid_argument);
  EXPECT_EQ(ToVector(CellMatrix::Row({3, 4}), "v"),
            (std::vector<double>{3, 4}));
}

TEST(RegistryTest, InvokePricingFunctions) {
  const auto& reg = FunctionRegistry::Pricing();
  const CellMatrix price =
      reg.Invoke("JoshiBlackScholesCall", {Cell(100.0), Cell(110.0), Cell(0.05),
                                           Cell(0.01), Cell(0.25), Cell(2.0)});
  const double expected = BlackScholesCall(100, 110, 0.05, 0.01, 0.25, 2);
  EXPECT_DOUBLE_EQ(std::get<double>(price(0, 0)), expected);
  const CellMatrix vol =
      reg.Invoke("JoshiImpliedVol", {Cell(expected), Cell(100.0), Cell(110.0),
                                     Cell(0.05), Cell(0.01), Cell(2.0)});
  EXPECT_NEAR(std::get<double>(vol(0, 0)), 0.25, 1e-9);
  const CellMatrix payoffs = reg.Invoke(
      "JoshiPayOff", {Cell(std::string("call")), Cell(100.0),
                      CellMatrix::Row({90, 100, 125})});
  ASSERT_EQ(payoffs.rows(), 3u);
  EXPECT_EQ(std::get<double>(payoffs(2, 0)), 25);
  const CellMatrix asian = reg.Invoke(
      "JoshiAsianCallMc", {Cell(100.0), Cell(100.0), Cell(0.05), Cell(0.0),
                           Cell(0.2), CellMatrix::Column({1.0}),
                           Cell(50000.0), Cell(1.0)});
  ASSERT_EQ(asian.cols(), 2u);
  EXPECT_NEAR(std::get<double>(asian(0, 0)),
              BlackScholesCall(100, 100, 0.05, 0, 0.2, 1),
              4 * std::get<double>(asian(0, 1)));
}

TEST(RegistryTest, ErrorsBecomeCellsNotExceptions) {
  const auto& reg = FunctionRegistry::Pricing();
  EXPECT_EQ(std::get<CellError>(reg.Invoke("NoSuchFunction", {})(0, 0)),
            CellError::kName);
  EXPECT_EQ(ErrorText(reg.Invoke("JoshiBlackScholesCall", {})),
            "#ERROR: expected 6 arguments, got 0");
  const std::string bad_vol = ErrorText(
      reg.Invoke("JoshiBlackScholesCall", {Cell(100.0), Cell(100.0), Cell(0.05),
                                           Cell(0.0), Cell(-0.2), Cell(1.0)}));
  EXPECT_NE(bad_vol.find("vol"), std::string::npos) << bad_vol;
  const std::string bad_name = ErrorText(reg.Invoke(
      "JoshiPayOff", {Cell(std::string("swap")), Cell(1.0), Cell(1.0)}));
  EXPECT_NE(bad_name.find("unknown id 'swap'"), std::string::npos) << bad_name;
}

TEST(CApiTest, StatusCodesAndLastError) {
  double out = 0;
  EXPECT_EQ(cpp4q_joshi_bs_call(100, 100, 0.05, 0, 0.2, 1, &out), 0);
  EXPECT_DOUBLE_EQ(out, BlackScholesCall(100, 100, 0.05, 0, 0.2, 1));
  EXPECT_STREQ(cpp4q_joshi_last_error(), "");
  EXPECT_EQ(cpp4q_joshi_bs_call(100, 100, 0.05, 0, 0.2, 1, nullptr), 1);
  EXPECT_STREQ(cpp4q_joshi_last_error(), "price is null");
  double vol = 0;
  EXPECT_EQ(cpp4q_joshi_implied_vol(out, 100, 100, 0.05, 0, 1, &vol), 0);
  EXPECT_NEAR(vol, 0.2, 1e-9);
  EXPECT_EQ(cpp4q_joshi_implied_vol(1000, 100, 100, 0.05, 0, 1, &vol), 2);
  const double spots[] = {90, 110};
  double values[2];
  EXPECT_EQ(cpp4q_joshi_payoff("put", 100, spots, 2, values), 0);
  EXPECT_EQ(values[0], 10);
  EXPECT_EQ(values[1], 0);
  EXPECT_EQ(cpp4q_joshi_payoff("bond", 100, spots, 2, values), 3);
}

}  // namespace
}  // namespace cpp4q::joshi::ch15
