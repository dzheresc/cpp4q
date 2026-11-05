#include <initializer_list>
#include <gtest/gtest.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "ch10_factory/payoff_factory.h"

namespace cpp4q::joshi::ch10 {
namespace {

TEST(PayOffFactoryTest, RegistrationsSurviveStaticLinking) {
  // Nothing in this test references payoff_registration.cc; it is linked
  // whole-archive, so its registrars still ran.
  const auto ids = PayOffFactory::Instance().RegisteredIds();
  for (const char* id : {"call", "put", "digital_call", "straddle"}) {
    EXPECT_NE(std::find(ids.begin(), ids.end(), id), ids.end()) << id;
  }
}

TEST(PayOffFactoryTest, CreatesByName) {
  auto& f = PayOffFactory::Instance();
  EXPECT_EQ((*f.CreatePayOff("call", 100))(130), 30);
  EXPECT_EQ((*f.CreatePayOff("put", 100))(70), 30);
  EXPECT_EQ((*f.CreatePayOff("straddle", 100))(70), 30);
  EXPECT_EQ((*f.CreatePayOff("digital_call", 100))(100.5), 1);
}

TEST(PayOffFactoryTest, UnknownIdNamesTheKnownOnes) {
  try {
    PayOffFactory::Instance().CreatePayOff("swaption", 1);
    FAIL();
  } catch (const std::out_of_range& e) {
    const std::string what = e.what();
    EXPECT_NE(what.find("swaption"), std::string::npos);
    EXPECT_NE(what.find("straddle"), std::string::npos);
  }
}

TEST(PayOffFactoryTest, DuplicateRegistrationKeepsFirst) {
  EXPECT_FALSE(PayOffFactory::Instance().RegisterPayOff(
      "call", &PayOffHelper<ch04::PayOffPut>::Create));
  EXPECT_EQ((*PayOffFactory::Instance().CreatePayOff("call", 100))(130), 30);
  // A new product registered at run time.
  EXPECT_TRUE(PayOffFactory::Instance().RegisterPayOff(
      "test_put_alias", &PayOffHelper<ch04::PayOffPut>::Create));
  EXPECT_EQ((*PayOffFactory::Instance().CreatePayOff("test_put_alias", 10))(4),
            6);
}

}  // namespace
}  // namespace cpp4q::joshi::ch10
