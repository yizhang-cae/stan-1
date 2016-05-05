#include <stan/services/config/validation/is_positive.hpp>
#include <gtest/gtest.h>

TEST(is_positive, validate) {
  stan::services::is_positive x;
  EXPECT_TRUE(x.validate(10.0));
  EXPECT_FALSE(x.validate(0.0));
  EXPECT_FALSE(x.validate(-10.0));

  EXPECT_TRUE(x.validate(10));
  EXPECT_FALSE(x.validate(0));
  EXPECT_FALSE(x.validate(-10));
}
