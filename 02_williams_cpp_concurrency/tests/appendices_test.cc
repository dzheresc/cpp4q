#include <gtest/gtest.h>

#include <map>
#include <string>
#include <variant>
#include <vector>

#include "appendices/mailbox.h"
#include "appendices/order_gateway.h"

namespace cpp4q::williams::appendix_c {
namespace {

TEST(MailboxTest, DispatchesByType) {
  Mailbox<int, std::string> box;
  box.Send(5);
  box.Send(std::string("hi"));
  std::string log;
  for (int i = 0; i < 2; ++i) {
    box.ReceiveAndHandle(Overloaded{
        [&](int x) { log += "int:" + std::to_string(x) + ";"; },
        [&](const std::string& s) { log += "str:" + s + ";"; },
    });
  }
  EXPECT_EQ(log, "int:5;str:hi;");
}

std::map<std::int64_t, std::vector<OrderStatus>> Collect(ClientMailbox& client,
                                                         int n) {
  std::map<std::int64_t, std::vector<OrderStatus>> by_id;
  for (int i = 0; i < n; ++i) {
    const auto r = std::get<ExecutionReport>(client.Receive());
    by_id[r.id].push_back(r.status);
  }
  return by_id;
}

TEST(OrderGatewayTest, LifecyclePerOrder) {
  ClientMailbox client;
  std::map<std::int64_t, std::vector<OrderStatus>> reports;
  {
    OrderGateway gw(client);
    gw.mailbox().Send(NewOrder{1, 500, 101.0});
    gw.mailbox().Send(NewOrder{2, 300, 99.0});
    gw.mailbox().Send(NewOrder{3, 50'000, 99.0});
    gw.mailbox().Send(NewOrder{4, 0, 99.0});
    reports = Collect(client, 9);  // 4 + 2 + 2 + 1
    gw.mailbox().Send(CancelOrder{2});
    auto more = Collect(client, 1);
    reports[2].push_back(more[2].at(0));
  }
  using S = OrderStatus;
  EXPECT_EQ(reports[1], (std::vector<S>{S::kPendingNew, S::kLive,
                                        S::kPartiallyFilled, S::kFilled}));
  EXPECT_EQ(reports[2],
            (std::vector<S>{S::kPendingNew, S::kLive, S::kCancelled}));
  EXPECT_EQ(reports[3], (std::vector<S>{S::kPendingNew, S::kRejected}));
  EXPECT_EQ(reports[4], (std::vector<S>{S::kRejected}));
}

TEST(OrderGatewayTest, CancelOfFilledOrderIsRejectedButStaysFilled) {
  ClientMailbox client;
  OrderGateway gw(client);
  gw.mailbox().Send(NewOrder{7, 10, 100.0});
  (void)Collect(client, 4);
  gw.mailbox().Send(CancelOrder{7});
  const auto r = std::get<ExecutionReport>(client.Receive());
  EXPECT_EQ(r.status, OrderStatus::kFilled);
  EXPECT_EQ(r.filled, 10);
  EXPECT_FALSE(r.text.empty());
}

TEST(OrderGatewayTest, ToString) {
  EXPECT_STREQ(ToString(OrderStatus::kPartiallyFilled), "PartiallyFilled");
}

}  // namespace
}  // namespace cpp4q::williams::appendix_c
