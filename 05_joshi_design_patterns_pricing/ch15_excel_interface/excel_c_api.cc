#include "ch15_excel_interface/excel_c_api.h"

#include <stdexcept>
#include <string>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch09_solvers_implied_vol/solvers.h"
#include "ch13_exceptions/exceptions.h"
#include "ch14_template_factory/registrations.h"

namespace {

namespace j = cpp4q::joshi;

thread_local std::string g_last_error;

template <class F>
int Guarded(F&& f) noexcept {
  g_last_error.clear();
  return static_cast<int>(
      j::ch13::CallNoThrow(std::forward<F>(f), &g_last_error));
}

void RequireNonNull(const void* p, const char* what) {
  if (p == nullptr) throw std::invalid_argument(std::string(what) + " is null");
}

}  // namespace

extern "C" {

int cpp4q_joshi_bs_call(double spot, double strike, double r, double d,
                        double vol, double expiry, double* price) {
  return Guarded([&] {
    RequireNonNull(price, "price");
    j::ch13::ValidateBsInputs(spot, strike, vol, expiry);
    *price = j::BlackScholesCall(spot, strike, r, d, vol, expiry);
  });
}

int cpp4q_joshi_implied_vol(double price, double spot, double strike, double r,
                            double d, double expiry, double* vol) {
  return Guarded([&] {
    RequireNonNull(vol, "vol");
    j::ch13::ValidateBsInputs(spot, strike, 0.0, expiry);
    *vol = j::ch09::ImpliedVolNewton(j::ch09::BsCall(r, d, expiry, spot, strike),
                                     price);
  });
}

int cpp4q_joshi_payoff(const char* name, double strike, const double* spots,
                       size_t n, double* values) {
  return Guarded([&] {
    RequireNonNull(name, "name");
    if (n > 0) {
      RequireNonNull(spots, "spots");
      RequireNonNull(values, "values");
    }
    const auto payoff =
        j::ch14::StrikePayOffFactory::Instance().Create(name, strike);
    for (size_t i = 0; i < n; ++i) values[i] = (*payoff)(spots[i]);
  });
}

const char* cpp4q_joshi_last_error(void) { return g_last_error.c_str(); }

}  // extern "C"
