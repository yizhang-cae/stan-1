functions {
  real fun_ord0(real t) {
    real res = 2.0;
    return res;
  }
  real fun_ord1(real t) {
    real k = 1.2;
    real res = k * t;
    return res;
  }
  real fun_ord2(real t) {
    real a = 2.3;
    real b = 2.0;
    real c = 1.5;
    real res = a + b * t + c * t * t;
    return res;
  }
}

data {
  real t0;
  real t1;
}

transformed data {
  real univar_integral;

  univar_integral = univariate_integral(fun_ord0, t0, t1);
  univar_integral = univariate_integral(fun_ord1, t0, t1);
  univar_integral = univariate_integral(fun_ord2, t0, t1);
}

parameters {
  real y_p;

  real y0_p[2];
  real theta_p[1];
}

transformed parameters {
}

model {
	y_p ~ normal(0,1);
}
