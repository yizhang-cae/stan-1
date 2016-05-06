#ifndef STAN_SERVICES_SAMPLE_SAMPLE_HPP
#define STAN_SERVICES_SAMPLE_SAMPLE_HPP

#include <stan/services/sample/sample_config.hpp>

namespace stan {
  namespace services {

    template <class Model>
    void sample(sample_config<Model>& config) { // add writers
      if (!config.validate()) {
        return;
      }
      
      if (config.algorithm_.val_ == "fixed_param") {
        // do fixed param
        return;
      }

      if (config.algorithm_.val_ == "hmc") {
        // do hmc
        return;
      }

      if (config.algorithm.val_ == "nuts") {
        // do nuts
        return;
      }
      
    }
  }
}
#endif
