// Chapter 7: Using C++ Classes.
//
// The first classes the book uses are the library's own: std::string,
// std::vector and the stream classes. Its running example is writing
// charts to a file a browser can open. Here a LineChart and a PieChart
// write self-contained HTML with inline SVG (no network access needed),
// plus CSV export; titles and labels are HTML-escaped.

#ifndef CPP4Q_ARMSTRONG_CH07_USING_CLASSES_CHARTS_H_
#define CPP4Q_ARMSTRONG_CH07_USING_CLASSES_CHARTS_H_

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace cpp4q::armstrong::ch07 {

// &, <, >, " and ' replaced by entities.
std::string HtmlEscape(std::string_view text);

// Two columns with a header row; throws if the sizes differ. Fields with
// commas, quotes or newlines are quoted per RFC 4180.
void WriteCsv(std::ostream& out, std::string_view x_name,
              std::string_view y_name, const std::vector<double>& x,
              const std::vector<double>& y);

class LineChart {
 public:
  explicit LineChart(std::string title) : title_(std::move(title)) {}
  // Adds a series; x and y must have the same non-zero length.
  void AddSeries(std::string name, std::vector<double> x,
                 std::vector<double> y);
  // Writes a complete HTML document.
  void WriteHtml(std::ostream& out) const;
  std::size_t series_count() const { return series_.size(); }

 private:
  struct Series {
    std::string name;
    std::vector<double> x;
    std::vector<double> y;
  };
  std::string title_;
  std::vector<Series> series_;
};

class PieChart {
 public:
  explicit PieChart(std::string title) : title_(std::move(title)) {}
  // Values must be non-negative.
  void AddEntry(std::string label, double value);
  void WriteHtml(std::ostream& out) const;

 private:
  std::string title_;
  std::vector<std::string> labels_;
  std::vector<double> values_;
};

}  // namespace cpp4q::armstrong::ch07

#endif  // CPP4Q_ARMSTRONG_CH07_USING_CLASSES_CHARTS_H_
