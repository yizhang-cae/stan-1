#ifndef STAN_SERVICES_CONFIG_VALIDATION_NOOP_HPP
#define STAN_SERVICES_CONFIG_VALIDATION_NOOP_HPP

namespace stan {
  namespace services {

    /**
     * Noop validation class.
     * 
     * This class allows any value to be
     * considered valid.
     */
    struct noop {

      /**
       * Validates a value.
       *
       * All values are considered valid.
       *
       * @tparam T type of value
       * @param x value to validate
       * @returns true
       */
      template <typename T>
      bool validate(const T& x) {
        return true;
      }
    };
      
  }
}

#endif
