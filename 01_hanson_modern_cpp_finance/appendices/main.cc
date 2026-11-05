// Appendices A-D in runnable form.
//
//  A. Virtual default destructor  - why the base needs one.
//  B. Object slicing              - what happens by value, and the fix.
//  C. Move special members        - see ch02_classes/price_buffer.{h,cc}.
//  D. vector initialisation       - braces vs parentheses.

#include <iostream>
#include <memory>
#include <vector>

#include "ch03_polymorphism/payoff.h"

namespace {

// --- Appendix A -----------------------------------------------------------
int g_destroyed = 0;

struct Base {
  virtual ~Base() = default;  // remove `virtual` and ~Derived never runs
  virtual double Value() const { return 0.0; }
};

struct Derived : Base {
  std::vector<double> payload = std::vector<double>(1024, 1.0);
  ~Derived() override { ++g_destroyed; }
  double Value() const override { return payload.front(); }
};

// --- Appendix B -----------------------------------------------------------
struct Instrument {
  virtual ~Instrument() = default;
  virtual const char* Name() const { return "Instrument"; }
};
struct Swap : Instrument {
  const char* Name() const override { return "Swap"; }
};

const char* NameByValue(Instrument i) { return i.Name(); }       // slices
const char* NameByRef(const Instrument& i) { return i.Name(); }  // dispatches

}  // namespace

int main() {
  {
    std::unique_ptr<Base> p = std::make_unique<Derived>();
    std::cout << "A: value " << p->Value() << '\n';
  }
  std::cout << "A: ~Derived ran " << g_destroyed << " time(s)\n";

  const Swap swap;
  std::cout << "B: by value -> " << NameByValue(swap) << ", by reference -> "
            << NameByRef(swap) << '\n';
  // ch03::Payoff has a protected copy constructor, so the slicing version
  // does not compile there:  ch03::Payoff p = ch03::CallPayoff(100);

  std::vector<double> a(3, 1.5);  // 3 copies of 1.5
  std::vector<double> b{3, 1.5};  // two elements: 3.0, 1.5
  std::cout << "D: (3, 1.5) size " << a.size() << ", {3, 1.5} size " << b.size()
            << '\n';
}
