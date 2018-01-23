functions {
  real[] fun_ord0(real t) {
    real res[1];
    res[1] = 2.0;
    return res;
  }
  real[] fun_ord1(real t) {
    real k = 1.2;
    real res[1];
    res[1] = k * t;
    return res;
  }
  real[] fun_ord2(real t) {
    real a = 2.3;
    real b = 2.0;
    real c = 1.5;
    real res[1];
    res[1] = a + b * t + c * t * t;
    return res;
  }
}

data {
  real dy0[1];
  real dtheta[1];
}

transformed data {
  real d_univar_integral[1];

  d_univar_integral = univariate_integral_rk45(fun_ord0, dy0, dtheta);
  d_univar_integral = univariate_integral_rk45(fun_ord1, dy0, dtheta);
  d_univar_integral = univariate_integral_rk45(fun_ord2, dy0, dtheta);

  d_univar_integral = univariate_integral_bdf(fun_ord0, dy0, dtheta);
  d_univar_integral = univariate_integral_bdf(fun_ord1, dy0, dtheta);
  d_univar_integral = univariate_integral_bdf(fun_ord2, dy0, dtheta);
}

parameters {
  real y_p;

  real py0[1];
  real ptheta[1];
}

transformed parameters {
  real p_univar_integral[1];

  p_univar_integral = univariate_integral_rk45(fun_ord0, py0, ptheta);
  p_univar_integral = univariate_integral_rk45(fun_ord1, py0, ptheta);
  p_univar_integral = univariate_integral_rk45(fun_ord2, py0, ptheta);

  p_univar_integral = univariate_integral_bdf(fun_ord0, py0, ptheta);
  p_univar_integral = univariate_integral_bdf(fun_ord1, py0, ptheta);
  p_univar_integral = univariate_integral_bdf(fun_ord2, py0, ptheta);
}

model {
	y_p ~ normal(0,1);
}
