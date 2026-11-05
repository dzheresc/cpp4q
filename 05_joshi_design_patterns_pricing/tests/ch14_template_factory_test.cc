#include <initializer_list>
#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "ch14_template_factory/registrations.h"

namespace cpp4q::joshi::ch14 {
namespace {

struct Shape {
  virtual ~Shape() = default;
  virtual double Area() const = 0;
};
struct Square : Shape {
  explicit Square(double s) : side(s) {}
  double Area() const override { return side * side; }
  double side;
};

TEST(TemplateFactoryTest, AnyBaseAnyArguments) {
  Factory<Shape, double> f;  // a private registry, not the global one
  EXPECT_TRUE(f.Register("square", [](double s) {
    return std::make_unique<Square>(s);
  }));
  EXPECT_FALSE(f.Register("square", [](double) { return nullptr; }));
  EXPECT_EQ(f.Create("square", 3)->Area(), 9);
  EXPECT_THROW(f.Create("circle", 1), std::out_of_range);
  EXPECT_TRUE(f.Unregister("square"));
  EXPECT_FALSE(f.Contains("square"));
  EXPECT_THROW(f.Register("x", nullptr), std::invalid_argument);
}

TEST(TemplateFactoryTest, GlobalRegistrations) {
  EXPECT_EQ((*StrikePayOffFactory::Instance().Create("put", 100))(60), 40);
  EXPECT_EQ((*RangePayOffFactory::Instance().Create("double_digital", 1, 2))(1.5),
            1);
  for (const char* id : {"park_miller", "mt19937_64", "halton",
                         "antithetic_park_miller", "antithetic_mt19937_64"}) {
    auto gen = RandomFactory::Instance().Create(id, 3, 17);
    ASSERT_NE(gen, nullptr) << id;
    EXPECT_EQ(gen->Dimensionality(), 3u);
    std::vector<double> u(3);
    gen->GetUniforms(u);
    for (const double x : u) {
      EXPECT_GT(x, 0.0) << id;
      EXPECT_LT(x, 1.0) << id;
    }
  }
}

TEST(TemplateFactoryTest, ConcurrentLookupsAndRegistrations) {
  Factory<Shape, double> f;
  f.Register("square", [](double s) { return std::make_unique<Square>(s); });
  std::atomic<int> created{0};
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; ++t) {
    threads.emplace_back([&f, &created] {
      for (int i = 0; i < 2000; ++i) {
        if (f.Create("square", 2)->Area() == 4) ++created;
      }
    });
  }
  threads.emplace_back([&f] {
    for (int i = 0; i < 500; ++i) {
      const std::string id = "tmp" + std::to_string(i);
      f.Register(id, [](double s) { return std::make_unique<Square>(s); });
      f.Unregister(id);
    }
  });
  for (auto& t : threads) t.join();
  EXPECT_EQ(created.load(), 8000);
  EXPECT_EQ(f.Keys(), std::vector<std::string>{"square"});
}

}  // namespace
}  // namespace cpp4q::joshi::ch14
