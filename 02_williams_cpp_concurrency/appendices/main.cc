// Appendix C demo: client -> gateway -> exchange, all message passing.

#include <cstdio>

#include "appendices/order_gateway.h"

int main() {
  namespace ac = cpp4q::williams::appendix_c;
  ac::ClientMailbox client;
  {
    ac::OrderGateway gateway(client);
    gateway.mailbox().Send(ac::NewOrder{1, 500, 101.0});    // marketable
    gateway.mailbox().Send(ac::NewOrder{2, 300, 99.0});     // rests
    gateway.mailbox().Send(ac::NewOrder{3, 50'000, 99.0});  // too big
    gateway.mailbox().Send(ac::CancelOrder{2});

    // 1: PendingNew, Live, PartiallyFilled, Filled; 2: PendingNew, Live,
    // Cancelled; 3: PendingNew, Rejected.
    for (int i = 0; i < 9; ++i) {
      const auto r = std::get<ac::ExecutionReport>(client.Receive());
      std::printf("order %lld %-16s filled %lld %s\n",
                  static_cast<long long>(r.id), ac::ToString(r.status),
                  static_cast<long long>(r.filled), r.text.c_str());
    }
  }
}
