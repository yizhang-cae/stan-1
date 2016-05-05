#ifndef STAN_SERVICES_CONFIG_VALIDATION_ONE_OF_HPP
#define STAN_SERVICES_CONFIG_VALIDATION_ONE_OF_HPP

#include <vector>

namespace stan {
  namespace services {

    /**
     * Validation class that checks that the value provide
     * is one of a list of possible values.
     *
     * @tparam T type of values.
     */
    template <typename T>
    struct one_of {
      std::vector<T> valid_;

      /**
       * Constructor with one valid value.
       *
       * @param valid the valid value.
       */
      explicit
      one_of(const T& valid) {
        valid_.push_back(valid);
      }

      /**
       * Constructor with two valid values.
       *
       * @param valid1 valid value
       * @param valid2 valid value
       */
      one_of(const T& valid1, const T& valid2) {
        valid_.push_back(valid1);
        valid_.push_back(valid2);
      }

      /**
       * Constructor with three valid values.
       *
       * @param valid1 valid value
       * @param valid2 valid value
       * @param valid3 valid value
       */
      one_of(const T& valid1, const T& valid2, const T& valid3) {
        valid_.push_back(valid1);
        valid_.push_back(valid2);
        valid_.push_back(valid3);
      }

      /**
       * Constructor with four valid values.
       *
       * @param valid1 valid value
       * @param valid2 valid value
       * @param valid3 valid value
       * @param valid4 valid value
       */
      one_of(const T& valid1, const T& valid2, const T& valid3,
             const T& valid4) {
        valid_.push_back(valid1);
        valid_.push_back(valid2);
        valid_.push_back(valid3);
        valid_.push_back(valid4);
      }

      /**
       * Validates that the value is one of the valid values
       * provided on construction.
       * 
       * @param x value to validate
       * @return true if the value is equal to one of the 
       *   valid values; false otherwise.
       */
      bool validate(const T& x) {
        for (size_t n = 0; n < valid_.size(); n++)
          if (valid_[n] == x)
            return true;
        return false;
      }
    };
      
  }
}

#endif
