#include "ch07_using_classes/charts.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <limits>
#include <numbers>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace cpp4q::armstrong::ch07 {
namespace {

constexpr double kWidth = 640;
constexpr double kHeight = 400;
constexpr double kMargin = 50;
constexpr const char* kPalette[] = {"#1f77b4", "#ff7f0e", "#2ca02c",
                                    "#d62728", "#9467bd", "#8c564b"};

std::string CsvField(std::string_view s) {
  if (s.find_first_of(",\"\n\r") == std::string_view::npos) {
    return std::string(s);
  }
  std::string out = "\"";
  for (char c : s) {
    if (c == '"') out += '"';
    out += c;
  }
  out += '"';
  return out;
}

void WriteHeader(std::ostream& out, const std::string& title) {
  out << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>"
      << HtmlEscape(title) << "</title></head><body>\n<h1>"
      << HtmlEscape(title) << "</h1>\n";
}

const char* Colour(std::size_t i) {
  return kPalette[i % std::size(kPalette)];
}

}  // namespace

std::string HtmlEscape(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}

void WriteCsv(std::ostream& out, std::string_view x_name,
              std::string_view y_name, const std::vector<double>& x,
              const std::vector<double>& y) {
  if (x.size() != y.size()) throw std::invalid_argument("size mismatch");
  out << CsvField(x_name) << ',' << CsvField(y_name) << '\n';
  for (std::size_t i = 0; i < x.size(); ++i) {
    out << std::format("{},{}\n", x[i], y[i]);
  }
}

void LineChart::AddSeries(std::string name, std::vector<double> x,
                          std::vector<double> y) {
  if (x.empty() || x.size() != y.size()) {
    throw std::invalid_argument("series needs equal, non-zero lengths");
  }
  series_.push_back({std::move(name), std::move(x), std::move(y)});
}

void LineChart::WriteHtml(std::ostream& out) const {
  WriteHeader(out, title_);
  double x_lo = std::numeric_limits<double>::infinity(), x_hi = -x_lo;
  double y_lo = x_lo, y_hi = -x_lo;
  for (const Series& s : series_) {
    const auto [xa, xb] = std::ranges::minmax(s.x);
    const auto [ya, yb] = std::ranges::minmax(s.y);
    x_lo = std::min(x_lo, xa);
    x_hi = std::max(x_hi, xb);
    y_lo = std::min(y_lo, ya);
    y_hi = std::max(y_hi, yb);
  }
  if (series_.empty()) x_lo = y_lo = 0, x_hi = y_hi = 1;
  if (x_hi == x_lo) x_hi = x_lo + 1;
  if (y_hi == y_lo) y_hi = y_lo + 1;
  const auto px = [&](double x) {
    return kMargin + (x - x_lo) / (x_hi - x_lo) * (kWidth - 2 * kMargin);
  };
  const auto py = [&](double y) {
    return kHeight - kMargin -
           (y - y_lo) / (y_hi - y_lo) * (kHeight - 2 * kMargin);
  };
  out << std::format(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"{}\" height=\"{}\">\n"
      "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n"
      "<line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\" stroke=\"black\"/>\n"
      "<line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\" stroke=\"black\"/>\n",
      kWidth, kHeight, kMargin, kHeight - kMargin, kWidth - kMargin,
      kHeight - kMargin, kMargin, kMargin, kMargin, kHeight - kMargin);
  out << std::format(
      "<text x=\"{}\" y=\"{}\" font-size=\"11\">{:.4g}</text>\n"
      "<text x=\"{}\" y=\"{}\" font-size=\"11\">{:.4g}</text>\n"
      "<text x=\"{}\" y=\"{}\" font-size=\"11\">{:.4g}</text>\n"
      "<text x=\"{}\" y=\"{}\" font-size=\"11\">{:.4g}</text>\n",
      kMargin, kHeight - kMargin + 15, x_lo, kWidth - kMargin,
      kHeight - kMargin + 15, x_hi, 2.0, kHeight - kMargin, y_lo, 2.0,
      kMargin, y_hi);
  for (std::size_t i = 0; i < series_.size(); ++i) {
    const Series& s = series_[i];
    out << "<polyline fill=\"none\" stroke=\"" << Colour(i)
        << "\" stroke-width=\"1.5\" points=\"";
    for (std::size_t j = 0; j < s.x.size(); ++j) {
      out << std::format("{:.2f},{:.2f} ", px(s.x[j]), py(s.y[j]));
    }
    out << "\"/>\n";
    out << std::format(
        "<text x=\"{}\" y=\"{}\" font-size=\"12\" fill=\"{}\">{}</text>\n",
        kWidth - kMargin - 120, kMargin + 15.0 * static_cast<double>(i),
        Colour(i), HtmlEscape(s.name));
  }
  out << "</svg>\n</body></html>\n";
}

void PieChart::AddEntry(std::string label, double value) {
  if (!(value >= 0)) throw std::invalid_argument("negative pie value");
  labels_.push_back(std::move(label));
  values_.push_back(value);
}

void PieChart::WriteHtml(std::ostream& out) const {
  WriteHeader(out, title_);
  double total = 0;
  for (double v : values_) total += v;
  constexpr double kCx = 200, kCy = 200, kR = 150;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"560\" "
         "height=\"400\">\n";
  double angle = 0;
  for (std::size_t i = 0; i < values_.size(); ++i) {
    const double frac = total > 0 ? values_[i] / total : 0;
    if (frac >= 1.0) {
      out << std::format(
          "<circle cx=\"{}\" cy=\"{}\" r=\"{}\" fill=\"{}\"/>\n", kCx, kCy, kR,
          Colour(i));
    } else if (frac > 0) {
      const double next = angle + 2 * std::numbers::pi * frac;
      out << std::format(
          "<path d=\"M {} {} L {:.3f} {:.3f} A {} {} 0 {} 1 {:.3f} {:.3f} Z\" "
          "fill=\"{}\"/>\n",
          kCx, kCy, kCx + kR * std::cos(angle), kCy + kR * std::sin(angle), kR,
          kR, frac > 0.5 ? 1 : 0, kCx + kR * std::cos(next),
          kCy + kR * std::sin(next), Colour(i));
      angle = next;
    }
    out << std::format(
        "<text x=\"380\" y=\"{}\" font-size=\"12\" fill=\"{}\">{} ({:.1f}%)"
        "</text>\n",
        40.0 + 18.0 * static_cast<double>(i), Colour(i),
        HtmlEscape(labels_[i]), 100 * frac);
  }
  out << "</svg>\n</body></html>\n";
}

}  // namespace cpp4q::armstrong::ch07
