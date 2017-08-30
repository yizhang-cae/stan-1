data {
  int d_int;
  int d2_int;
  int d3_int;
  real d_real_array[d_int];
  real d2_real_array[d2_int];
  real d3_real_array[d3_int];
}
transformed data {
  real transformed_data_real_array[d_int];

  transformed_data_real_array <- linear_interpolation(d_real_array, d2_real_array, d3_real_array);
}
parameters {
  real p_real_array[d_int];
  real p2_real_array[d2_int];
  real p3_real_array[d3_int];
  real y_p;
}
transformed parameters {
  real transformed_param_real_array[d_int];

  transformed_param_real_array <- linear_interpolation(d_real_array, d2_real_array, d3_real_array);
  transformed_param_real_array <- linear_interpolation(d_real_array, d2_real_array, p3_real_array);
  transformed_param_real_array <- linear_interpolation(d_real_array, p2_real_array, d3_real_array);
  transformed_param_real_array <- linear_interpolation(d_real_array, p2_real_array, p3_real_array);
  transformed_param_real_array <- linear_interpolation(p_real_array, d2_real_array, d3_real_array);
  transformed_param_real_array <- linear_interpolation(p_real_array, d2_real_array, p3_real_array);
  transformed_param_real_array <- linear_interpolation(p_real_array, p2_real_array, d3_real_array);
  transformed_param_real_array <- linear_interpolation(p_real_array, p2_real_array, p3_real_array);
}
model {  
  y_p ~ normal(0,1);
}
