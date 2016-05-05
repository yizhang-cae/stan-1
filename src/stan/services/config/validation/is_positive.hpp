#ifndef STAN_SERVICES_CONFIG_VALIDATION_IS_POSITIVE_HPP
#define STAN_SERVICES_CONFIG_VALIDATION_IS_POSITIVE_HPP

namespace stan {
  namespace services {

    /**
     * Validation class that validates that the value is positive.
     */
    struct is_positive {

      /**
       * Validates the value is greater than 0.
       *
       * This value checks that the value is greater than 0.
       *
       * @tparam T type of value
       * @param x value to validate
       * @returns true if x > 0 (using the greater than operator);
       *   false otherwise.
       */
      template <typename T>
      bool validate(const T& x) {
        return x > 0;
      }
    };

  }
}

#endif
