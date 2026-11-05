#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "ch07_using_classes/charts.h"

namespace cpp4q::armstrong::ch07 {
namespace {

std::size_t Count(const std::string& s, const std::string& what) {
  std::size_t n = 0;
  for (auto pos = s.find(what); pos != std::string::npos;
       pos = s.find(what, pos + 1)) {
    ++n;
  }
  return n;
}

TEST(ChartsTest, Escape) {
  EXPECT_EQ(HtmlEscape("a<b & \"c\" 'd'>"),
            "a&lt;b &amp; &quot;c&quot; &#39;d&#39;&gt;");
}

TEST(ChartsTest, Csv) {
  std::ostringstream out;
  WriteCsv(out, "t", "price, close", {0, 1}, {100, 101.5});
  EXPECT_EQ(out.str(), "t,\"price, close\"\n0,100\n1,101.5\n");
  EXPECT_THROW(WriteCsv(out, "a", "b", {1}, {1, 2}), std::invalid_argument);
}

TEST(ChartsTest, LineChartHasOnePolylinePerSeries) {
  LineChart chart("Rates <1y>");
  chart.AddSeries("a", {0, 1, 2}, {1, 4, 9});
  chart.AddSeries("b", {0, 2}, {5, 5});
  std::ostringstream out;
  chart.WriteHtml(out);
  const std::string html = out.str();
  EXPECT_EQ(Count(html, "<polyline"), 2u);
  EXPECT_NE(html.find("Rates &lt;1y&gt;"), std::string::npos);
  EXPECT_EQ(html.find("<1y>"), std::string::npos);
  EXPECT_NE(html.find("</html>"), std::string::npos);
  EXPECT_THROW(chart.AddSeries("bad", {1, 2}, {1}), std::invalid_argument);
  EXPECT_THROW(chart.AddSeries("empty", {}, {}), std::invalid_argument);
}

TEST(ChartsTest, PieChartSlices) {
  PieChart pie("Mix");
  pie.AddEntry("A", 1);
  pie.AddEntry("B", 3);
  pie.AddEntry("zero", 0);
  std::ostringstream out;
  pie.WriteHtml(out);
  EXPECT_EQ(Count(out.str(), "<path"), 2u);
  EXPECT_NE(out.str().find("B (75.0%)"), std::string::npos);
  PieChart whole("one");
  whole.AddEntry("all", 2);
  std::ostringstream one;
  whole.WriteHtml(one);
  EXPECT_EQ(Count(one.str(), "<circle"), 1u);
  EXPECT_THROW(pie.AddEntry("neg", -1), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch07
