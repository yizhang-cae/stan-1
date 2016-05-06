#include <stan/services/sample/sample.hpp>
#include <gtest/gtest.h>

class Model {
};

TEST(sample, test) {
  Model model;
  
  stan::services::sample_config<Model> config(model);

  stan::services::sample(config);
  
}
