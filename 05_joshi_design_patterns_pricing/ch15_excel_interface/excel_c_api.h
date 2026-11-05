/* Chapter 15: a C ABI over the pricers.
 *
 * What an XLL, a VBA Declare, Python ctypes or any other language can call:
 * plain C types, a status code instead of exceptions, results through out
 * pointers, and the last error message per thread. Status codes are those
 * of cpp4q::joshi::ch13::Status (0 = success). */

#ifndef CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_EXCEL_C_API_H_
#define CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_EXCEL_C_API_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int cpp4q_joshi_bs_call(double spot, double strike, double r, double d,
                        double vol, double expiry, double* price);
int cpp4q_joshi_implied_vol(double price, double spot, double strike, double r,
                            double d, double expiry, double* vol);
/* Evaluates the pay-off `name` ("call", "put", "digital_call") at n spots. */
int cpp4q_joshi_payoff(const char* name, double strike, const double* spots,
                       size_t n, double* values);
/* The message of the last failed call on this thread ("" if none). Valid
 * until the next call on this thread. */
const char* cpp4q_joshi_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* CPP4Q_JOSHI_CH15_EXCEL_INTERFACE_EXCEL_C_API_H_ */
