functions {
  real[] ode(real t,
             real[] y,
             real[] y_pk,
             real[] theta,
             real[] x,
             int[] x_int) {
    real dydt[2];
    return dydt;
  }
}

data {
  int<lower = 1> nt;
  int nTheta;
  int<lower = 1> cmt[nt];
  int evid[nt];
  int addl[nt];
  int ss[nt];
  real amt[nt];
  real time[nt];
  real rate[nt];
  real ii[nt];
}

transformed data {
  int nOde = 2;
  int nCmt = 4;
  real theta_data[nt, nTheta];
  real biovar_data[nt, nCmt];
  real tlag_data[nt, nCmt];
  matrix[nt, nCmt] x_data;

  // RK45
  x_data = mixOde1CptModel_rk45(ode, nOde,
                                time, amt, rate, ii, evid, cmt, addl, ss,
                                theta_data, biovar_data, tlag_data,
                                1e-8, 1e-8, 1e8);
}

parameters {
  real y_p;

  real y0_p[2];
  real theta_p[1];
}

transformed parameters {
  real theta_parm[nt, nTheta];
  real biovar_parm[nt, nCmt];
  real tlag_parm[nt, nCmt];
  matrix[nt, nCmt] x_parm;

  // RK45

  // BDF

}

model {
	y_p ~ normal(0,1);
}
