#include <stan/services/config/value.hpp>
#include <gtest/gtest.h>

struct mock_validation {
  int count_;
  bool next_validate_;

  mock_validation()
  : count_(0),
    next_validate_(true) {
  }

  template <typename T>
  bool validate(const T& x) {
    count_++;
    return next_validate_;
  }
};

TEST(value, construct) {
  stan::services::value<int, mock_validation> value1;
  EXPECT_FALSE(value1.is_set_);
  EXPECT_FALSE(value1.has_default_);
  EXPECT_TRUE(value1.validate());
  EXPECT_FALSE(value1.validate_exists());
  
  stan::services::value<int, mock_validation> value2(10);
  EXPECT_FALSE(value2.is_set_);
  EXPECT_TRUE(value2.has_default_);
  EXPECT_EQ(10, value2.default_);
  EXPECT_TRUE(value2.validate());
  EXPECT_FALSE(value2.validate_exists());

  mock_validation validation1;
  stan::services::value<int, mock_validation> value3(validation1);
  EXPECT_FALSE(value3.is_set_);
  EXPECT_FALSE(value3.has_default_);
  EXPECT_TRUE(value3.validate());
  EXPECT_FALSE(value3.validate_exists());
  EXPECT_EQ(0, validation1.count_);
  
  mock_validation validation2;
  stan::services::value<int, mock_validation> value4(10, validation2);
  EXPECT_FALSE(value4.is_set_);
  EXPECT_TRUE(value4.has_default_);
  EXPECT_EQ(10, value4.default_);
  EXPECT_TRUE(value4.validate());
  EXPECT_EQ(0, validation2.count_);
  EXPECT_FALSE(value4.validate_exists());
  EXPECT_EQ(0, validation2.count_);
}
