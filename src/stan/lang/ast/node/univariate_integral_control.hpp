#ifndef STAN_LANG_AST_NODE_UNIVARIATE_INTEGRAL_CONTROL_HPP
#define STAN_LANG_AST_NODE_UNIVARIATE_INTEGRAL_CONTROL_HPP

#include <stan/lang/ast/node/expression.hpp>
#include <string>

namespace stan {
  namespace lang {

    struct expression;

    /**
     * Structure for a Torsten univariate_integral function with control
     * parameters for the integrator.
     */
    struct univariate_integral_control {
      /**
       * Name of the functor that's be integrated
       */
      std::string system_function_name_;

      /**
       * init condition
       */
      expression y0_;

      /**
       * parameters
       */
      expression theta_;

      /**
       * default constructor.
       */
      univariate_integral_control();

      /**
       * Construct an univarate integral
       *
       * @param f functor for that's being integrated
       * @param t0 left end of interval
       * @param t1 right end of interval
       */
      univariate_integral_control(const
                                  std::string& system_function_name,
                                  const expression& y0,
                                  const expression& theta);
    };

  }
}
#endif

