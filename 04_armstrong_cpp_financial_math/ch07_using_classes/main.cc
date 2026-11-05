// Chapter 7 demo: strings, vectors and file streams. Writes a line chart
// and a pie chart to HTML files in the current directory (or the
// directory given as the first argument).

#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "ch07_using_classes/charts.h"

namespace a = cpp4q::armstrong::ch07;

int main(int argc, char** argv) {
  const std::string dir = argc > 1 ? argv[1] : ".";
  std::vector<double> t, simple, continuous;
  for (int i = 0; i <= 30; ++i) {
    t.push_back(i);
    simple.push_back(100 * (1 + 0.05 * i));
    continuous.push_back(100 * std::exp(0.05 * i));
  }
  a::LineChart line("Growth of 100 at 5%: simple vs continuous");
  line.AddSeries("simple", t, simple);
  line.AddSeries("continuous", t, continuous);
  {
    std::ofstream out(dir + "/growth.html");
    line.WriteHtml(out);
  }
  a::PieChart pie("Portfolio by asset class");
  pie.AddEntry("Equities", 55);
  pie.AddEntry("Bonds", 30);
  pie.AddEntry("Cash & other", 15);
  std::ofstream pie_out(dir + "/allocation.html");
  pie.WriteHtml(pie_out);
  std::printf("wrote %s/growth.html and %s/allocation.html\n", dir.c_str(),
              dir.c_str());
  return 0;
}
