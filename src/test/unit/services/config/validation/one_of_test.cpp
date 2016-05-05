#include <stan/services/config/validation/one_of.hpp>
#include <gtest/gtest.h>

TEST(one_of, validate) {
  stan::services::one_of<std::string> x("one", "two", "three");
  EXPECT_TRUE(x.validate("one"));
  EXPECT_TRUE(x.validate("two"));
  EXPECT_TRUE(x.validate("three"));
  EXPECT_FALSE(x.validate("four"));

  stan::services::one_of<int> y(1, 2, 3);
  EXPECT_TRUE(y.validate(1));
  EXPECT_TRUE(y.validate(2));
  EXPECT_TRUE(y.validate(3));
  EXPECT_FALSE(y.validate(4));
}
