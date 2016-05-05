#ifndef STAN_SERVICES_SAMPLE_SAMPLE_CONFIG_HPP
#define STAN_SERVICES_SAMPLE_SAMPLE_CONFIG_HPP

#include <stan/services/config/value.hpp>
#include <stan/services/config/validation/is_positive.hpp>
#include <stan/services/config/validation/one_of.hpp>
#include <string>

namespace stan {
  namespace services {
    
    // template <class Model>
    struct sample_config {
      // const Model& model_;

      // var_context& data_;

      // var_context& fixed_inits_;
      typedef bool do_random_inits_t;
      value<do_random_inits_t> do_random_inits_;
      typedef double random_init_bounds_t;
      value<random_init_bounds_t, is_positive> random_init_bounds_;

      typedef std::string metric_t;
      value<metric_t, one_of<std::string> > metric_;
      
      // value<VectorXd, ...> init_diag_metric_;
      // value<MatrixXd, ...> init_dense_metric_;

      typedef int max_treedepth_t;
      value<max_treedepth_t, is_positive> max_treedepth_;
      typedef double int_time_t;
      value<int_time_t, is_positive> int_time_;

      typedef int iter_t;
      value<iter_t, is_positive> iter_;
      typedef int warmup_t;
      value<warmup_t, is_positive> warmup_; // iter/2
      typedef int num_samples_t;
      value<num_samples_t, is_positive> num_samples_;
      typedef int thin_t;
      value<thin_t, is_positive> thin_;

      
      // explicit sample_config(Model& m)
      // : model_(m)
      sample_config()
        : do_random_inits_(true),
          random_init_bounds_(2),
          metric_("diag_e", one_of<std::string>("dense_e", "diag_e", "unit_e")),
          max_treedepth_(10),
          int_time_(1),
          iter_(2000),
          warmup_(iter_.default_ / 2),
          num_samples_(1000),
          thin_(1)
      { }
      
      void validate() {
        // individual validation
        do_random_inits_.validate();
        random_init_bounds_.validate();
        metric_.validate();
        max_treedepth_.validate();
        int_time_.validate();
        iter_.validate();
        warmup_.validate();
        num_samples_.validate();
        thin_.validate();
        max_treedepth_.validate();
        int_time_.validate();

        //   if (algorithm_.value_ == "hmc") {
        //     if (max_treedepth_.is_set_) barf;
        //     ...
        //       }
        
        //   if (algorithm_.value_ == "nuts") {
        //     if (int_time_.is_set_) barf;
        //     ...
        //       }
        
        //   if (adapt_.value_ == true) {
        //     if (adapt_engaged_.is_set_) barf;
        //     ...
        
        //       }

      }

    };
    
  }
}

#endif
