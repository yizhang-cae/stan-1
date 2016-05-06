#ifndef STAN_SERVICES_SAMPLE_SAMPLE_CONFIG_HPP
#define STAN_SERVICES_SAMPLE_SAMPLE_CONFIG_HPP

#include <stan/services/config/value.hpp>
#include <stan/services/config/validation/is_positive.hpp>
#include <stan/services/config/validation/one_of.hpp>
#include <string>

namespace stan {
  namespace services {

    /**
     * Configuation for the sample function.
     *
     * @tparam Model model class that follows the Stan program concept
     */
    template <class Model>
    struct sample_config {
      const Model& model_;

      // var_context& data_;

      // var_context& fixed_inits_;
      typedef bool do_random_inits_t;
      value<do_random_inits_t> do_random_inits_;
      typedef double random_init_bounds_t;
      value<random_init_bounds_t, is_positive> random_init_bounds_;

      typedef std::string algorithm_t;
      value<algorithm_t, one_of<std::string> > algorithm_;
      
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
      value<warmup_t, is_positive> warmup_;
      typedef int num_samples_t;
      value<num_samples_t, is_positive> num_samples_;
      typedef int thin_t;
      value<thin_t, is_positive> thin_;

      /**
       * Constructor.
       *
       * Creates the sample configuration with the defaults.
       *
       * @param model The Stan program
       */
       explicit sample_config(Model& model)
         : model_(model),
           do_random_inits_(true),
           random_init_bounds_(2),
           algorithm_("nuts", one_of<std::string>("hmc", "nuts", "fixed_param")),
           metric_("diag_e", one_of<std::string>("dense_e", "diag_e", "unit_e")),
           max_treedepth_(10),
           int_time_(1),
           iter_(2000),
           warmup_(iter_.default_ / 2),
           num_samples_(1000),
           thin_(1)
      { }

      /**
       * Validates the input.
       *
       * @return true if the sample object is ok;
       *   false otherwise. May write to the writer.
       */
      bool validate() {
        // individual validation
        if (!do_random_inits_.validate()) return false;
        if (!random_init_bounds_.validate()) return false;
        if (!metric_.validate()) return false;
        if (!max_treedepth_.validate()) return false;
        if (!int_time_.validate()) return false;
        if (!iter_.validate()) return false;
        if (!warmup_.validate()) return false;
        if (!num_samples_.validate()) return false;
        if (!thin_.validate()) return false;
        if (!max_treedepth_.validate()) return false;
        if (!int_time_.validate()) return false;

        if (algorithm_.val_ == "hmc") {
          if (max_treedepth_.is_set_) {
            // barf;
            return false;
          }
        }

        
        if (algorithm_.val_ == "nuts") {
          if (int_time_.is_set_) {
            // barf;
            return false;
          }
        }

        /*
         if (adapt_.val_ == true) {
          if (adapt_engaged_.is_set_) {
            // barf;
            return false;
          }
        }
        */
        return true;
      }

    };
    
  }
}
#endif
