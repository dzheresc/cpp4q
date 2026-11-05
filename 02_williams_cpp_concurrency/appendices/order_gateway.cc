#include "appendices/order_gateway.h"

#include <utility>

namespace cpp4q::williams::appendix_c {

const char* ToString(OrderStatus s) {
  switch (s) {
    case OrderStatus::kPendingNew:
      return "PendingNew";
    case OrderStatus::kLive:
      return "Live";
    case OrderStatus::kPartiallyFilled:
      return "PartiallyFilled";
    case OrderStatus::kFilled:
      return "Filled";
    case OrderStatus::kCancelled:
      return "Cancelled";
    case OrderStatus::kRejected:
      return "Rejected";
  }
  return "?";
}

Exchange::Exchange(GatewayMailbox& gateway, double market_price,
                   std::int64_t max_quantity)
    : gateway_(gateway),
      market_price_(market_price),
      max_quantity_(max_quantity),
      thread_([this] { Run(); }) {}

void Exchange::Run() {
  bool running = true;
  while (running) {
    inbox_.ReceiveAndHandle(Overloaded{
        [&](const ExchNew& m) {
          if (m.quantity > max_quantity_) {
            gateway_.Send(ExchReject{m.id, "quantity above venue maximum"});
            return;
          }
          gateway_.Send(ExchAck{m.id});
          if (m.limit_price >= market_price_) {
            const std::int64_t first = m.quantity / 2;
            if (first > 0) gateway_.Send(ExchFill{m.id, first, market_price_});
            gateway_.Send(ExchFill{m.id, m.quantity - first, market_price_});
          } else {
            resting_[m.id] = m.quantity;
          }
        },
        [&](const ExchCancel& m) {
          if (resting_.erase(m.id) != 0) {
            gateway_.Send(ExchCancelled{m.id});
          } else {
            gateway_.Send(ExchReject{m.id, "unknown or filled order"});
          }
        },
        [&](const Shutdown&) { running = false; },
    });
  }
}

OrderGateway::OrderGateway(ClientMailbox& client)
    : client_(client),
      exchange_(inbox_, /*market_price=*/100.0, /*max_quantity=*/10'000),
      thread_([this] { Run(); }) {}

OrderGateway::~OrderGateway() { inbox_.Send(Shutdown{}); }

void OrderGateway::Report(std::int64_t id, const OrderState& s,
                          std::string text) {
  client_.Send(ExecutionReport{id, s.status, s.filled, std::move(text)});
}

void OrderGateway::Run() {
  bool running = true;
  while (running) {
    inbox_.ReceiveAndHandle(Overloaded{
        [&](const NewOrder& m) {
          if (m.quantity <= 0 || orders_.contains(m.id)) {
            Report(m.id, {.status = OrderStatus::kRejected},
                   "rejected by gateway");
            return;
          }
          auto& s = orders_[m.id];
          s.quantity = m.quantity;
          Report(m.id, s);
          exchange_.mailbox().Send(ExchNew{m.id, m.quantity, m.limit_price});
        },
        [&](const CancelOrder& m) {
          exchange_.mailbox().Send(ExchCancel{m.id});
        },
        [&](const ExchAck& m) {
          auto& s = orders_[m.id];
          s.status = OrderStatus::kLive;
          Report(m.id, s);
        },
        [&](const ExchFill& m) {
          auto& s = orders_[m.id];
          s.filled += m.quantity;
          s.status = s.filled >= s.quantity ? OrderStatus::kFilled
                                            : OrderStatus::kPartiallyFilled;
          Report(m.id, s);
        },
        [&](const ExchReject& m) {
          auto it = orders_.find(m.id);
          if (it == orders_.end()) return;
          // A reject of a cancel leaves a filled order filled.
          if (it->second.status == OrderStatus::kPendingNew) {
            it->second.status = OrderStatus::kRejected;
          }
          Report(m.id, it->second, m.reason);
        },
        [&](const ExchCancelled& m) {
          auto& s = orders_[m.id];
          s.status = OrderStatus::kCancelled;
          Report(m.id, s);
        },
        [&](const Shutdown&) {
          exchange_.mailbox().Send(Shutdown{});
          running = false;
        },
    });
  }
}

}  // namespace cpp4q::williams::appendix_c
