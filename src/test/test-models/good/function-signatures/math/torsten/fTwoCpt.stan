data {
  real dt;
  int nParameters;
  vector[nParameters] parameter;
  int nCmt;
  vector[nCmt] init;
  real rate[nCmt];
}

transformed data {
  vector[nCmt] pred;
  pred = fTwoCpt(dt, parameter, init, rate);

}

parameters {
  real y_p;
}

transformed parameters {
  real dt_p;
  vector[nParameters] parameter_p;
  vector[nCmt] init_p;
  // currently rate is expected to be data
  vector[nCmt] pred_p;

  pred_p = fTwoCpt(dt, parameter, init, rate);
  pred_p = fTwoCpt(dt, parameter_p, init_p, rate);

  // For now, only tests interesting scenarios, rather than
  // all possible combinations.

}

model {
	y_p ~ normal(0,1);
}
