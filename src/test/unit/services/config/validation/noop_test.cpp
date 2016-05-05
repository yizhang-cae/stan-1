#include <stan/services/config/validation/noop.hpp>
#include <gtest/gtest.h>

TEST(noop, validate) {
  stan::services::noop x;
  EXPECT_TRUE(x.validate(10.0));
  EXPECT_TRUE(x.validate(0.0));
  EXPECT_TRUE(x.validate(-10.0));

  EXPECT_TRUE(x.validate(10));
  EXPECT_TRUE(x.validate(0));
  EXPECT_TRUE(x.validate(-10));
}
