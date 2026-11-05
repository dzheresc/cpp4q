// The registrations. Nothing references these objects; they exist only for
// the side effect of their constructors at program start-up.

#include "ch04_virtual_constructor/payoff3.h"
#include "ch10_factory/payoff_factory.h"

namespace cpp4q::joshi::ch10 {
namespace {

const PayOffHelper<ch04::PayOffCall> kRegisterCall("call");
const PayOffHelper<ch04::PayOffPut> kRegisterPut("put");
const PayOffHelper<ch04::PayOffDigitalCall> kRegisterDigital("digital_call");
const PayOffHelper<PayOffStraddle> kRegisterStraddle("straddle");

}  // namespace
}  // namespace cpp4q::joshi::ch10
