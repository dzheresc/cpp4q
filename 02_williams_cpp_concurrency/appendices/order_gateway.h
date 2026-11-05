// Appendix C, finance edition: instead of the book's ATM, an order gateway
// and a simulated exchange, each an actor on its own thread that only
// talks through mailboxes. No state is shared, so there are no locks
// outside the mailboxes themselves.
//
//   client --NewOrder/Cancel--> gateway --ExchNew/ExchCancel--> exchange
//   client <--ExecutionReport-- gateway <--Ack/Fill/Reject/Cancelled--

#ifndef CPP4Q_WILLIAMS_APPENDICES_ORDER_GATEWAY_H_
#define CPP4Q_WILLIAMS_APPENDICES_ORDER_GATEWAY_H_

#include <cstdint>
#include <map>
#include <string>
#include <thread>

#include "appendices/mailbox.h"

namespace cpp4q::williams::appendix_c {

enum class OrderStatus {
  kPendingNew,
  kLive,
  kPartiallyFilled,
  kFilled,
  kCancelled,
  kRejected
};
const char* ToString(OrderStatus s);

// Client -> gateway.
struct NewOrder {
  std::int64_t id;
  std::int64_t quantity;
  double limit_price;
};
struct CancelOrder {
  std::int64_t id;
};
struct Shutdown {};

// Gateway -> exchange.
struct ExchNew {
  std::int64_t id;
  std::int64_t quantity;
  double limit_price;
};
struct ExchCancel {
  std::int64_t id;
};

// Exchange -> gateway.
struct ExchAck {
  std::int64_t id;
};
struct ExchFill {
  std::int64_t id;
  std::int64_t quantity;
  double price;
};
struct ExchReject {
  std::int64_t id;
  std::string reason;
};
struct ExchCancelled {
  std::int64_t id;
};

// Gateway -> client.
struct ExecutionReport {
  std::int64_t id;
  OrderStatus status;
  std::int64_t filled;
  std::string text;
};

using GatewayMailbox = Mailbox<NewOrder, CancelOrder, Shutdown, ExchAck,
                               ExchFill, ExchReject, ExchCancelled>;
using ExchangeMailbox = Mailbox<ExchNew, ExchCancel, Shutdown>;
using ClientMailbox = Mailbox<ExecutionReport>;

// Simulated venue: rejects orders above `max_quantity`; orders with a limit
// at or above `market_price` fill in two halves; others rest until
// cancelled.
class Exchange {
 public:
  Exchange(GatewayMailbox& gateway, double market_price,
           std::int64_t max_quantity);
  ExchangeMailbox& mailbox() { return inbox_; }

 private:
  void Run();
  GatewayMailbox& gateway_;
  const double market_price_;
  const std::int64_t max_quantity_;
  ExchangeMailbox inbox_;
  std::map<std::int64_t, std::int64_t> resting_;  // id -> open qty
  std::jthread thread_;                           // last member
};

class OrderGateway {
 public:
  explicit OrderGateway(ClientMailbox& client);
  ~OrderGateway();  // sends Shutdown and joins
  GatewayMailbox& mailbox() { return inbox_; }

 private:
  struct OrderState {
    std::int64_t quantity = 0;
    std::int64_t filled = 0;
    OrderStatus status = OrderStatus::kPendingNew;
  };
  void Run();
  void Report(std::int64_t id, const OrderState& s, std::string text = {});

  ClientMailbox& client_;
  GatewayMailbox inbox_;
  Exchange exchange_;
  std::map<std::int64_t, OrderState> orders_;  // touched only by thread_
  std::jthread thread_;                        // last member
};

}  // namespace cpp4q::williams::appendix_c

#endif  // CPP4Q_WILLIAMS_APPENDICES_ORDER_GATEWAY_H_
