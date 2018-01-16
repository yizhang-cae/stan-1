#ifndef STAN_LANG_AST_NODE_UNIVARIATE_INTEGRAL_CONTROL_DEF_HPP
#define STAN_LANG_AST_NODE_UNIVARIATE_INTEGRAL_CONTROL_DEF_HPP

#include <stan/lang/ast.hpp>
#include <string>

namespace stan {
  namespace lang {

    univariate_integral_control::univariate_integral_control() { }

    univariate_integral_control::
    univariate_integral_control(const std::string& system_function_name,
                                const expression& y0,
                                const expression& theta)
      : system_function_name_(system_function_name),
        y0_(y0),
        theta_(theta)
    { }
  }
}
#endif
