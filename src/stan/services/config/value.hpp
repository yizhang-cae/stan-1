#ifndef STAN_SERVICES_CONFIG_VALUE_HPP
#define STAN_SERVICES_CONFIG_VALUE_HPP

#include <stan/services/config/validation/noop.hpp>
#include <stdexcept>

namespace stan {
  namespace services {
    
    /**
     * Holds a value for use in configuration.
     *
     * The value class holds a value, has an optional default value,
     * an indicator for whether the value has been set, and it accepts
     * a class for validating the input. The value can only be set once.
     *
     * @tparam T Type of value
     * @tparam V Validation class. Defaults to noop validation.
     */
    template <typename T, class V = noop>
    struct value {
      T val_;
      bool is_set_;
      const T default_;
      const bool has_default_;
      V v_;

      /**
       * Default constructor.
       * 
       * This constructs the value without a default value and uses
       * the noop validation class.
       */
      value()
        : is_set_(false),
          default_(),
          has_default_(false),
          v_() { }

      /**
       * Constructor.
       *
       * This constructs the value without a default value and the
       * uses the validation class instance provided.
       * 
       * @param v validator
       */
      explicit value(const V& v)
        : is_set_(false),
          default_(),
          has_default_(false),
          v_(v) { }

      /**
       * Constructor. 
       *
       * This constructs the value with a default value and uses
       * the noop validation class.
       *
       * @param d default value
       */
      explicit value(const T& d)
        : is_set_(false),
          default_(d),
          has_default_(true),
          v_() { }

      /**
       * Constructor.
       *
       * This constructs the value with a default value and uses the
       * validation class instance provided.
       *
       * @param d default value
       * @param v validator
       * @throw std::invalid_argument if the default value is not a valid
       *   value.
       */
      value(const T& d, const V& v)
        : is_set_(false),
          default_(d),
          has_default_(true),
          v_(v) {
        if (!v_.validate(default_))
          throw std::invalid_argument("default value is not valid");
      }

      /**
       * Sets the value.
       *
       * Sets the value to the one provided. This method should only
       * be called once on the object.
       *
       * @param val value to set
       * @throw std::runtime_error if the value has previously been set.
       */
      void set(const T& val) {
        if (is_set_) {
          throw std::runtime_error("value is set twice");
        }
        is_set_ = true;
        val_ = val;
      }

      /**
       * Indicates whether the value is valid.
       *
       * If the value isn't set, the value is assumed to be valid.
       * If the value is set, the validation class is used to
       * validate the value.
       * 
       * @return true if the value is valid or is not set; 
       *   false otherwise.
       */
      bool validate() {
        if (!is_set_)
          return true;
        return v_.validate(val_);
      }

      /**
       * Indicates whether the value is valid and it has been set.
       *
       * @return true if the value is valid and is set;
       *  false otherwise.
       */
      bool validate_exists() {
        return is_set_ && v_.validate(val_);
      }
    };

  }
}
#endif
