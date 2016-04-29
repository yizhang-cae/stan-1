data {
  vector[5] x;
}
parameters{
  real a;
}
model {
  vector[5] y;
  y = x ./ a;
}
