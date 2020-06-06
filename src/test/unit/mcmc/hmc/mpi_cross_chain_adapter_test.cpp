#ifdef MPI_ADAPTED_WARMUP

#include <gtest/gtest.h>
#include <stan/mcmc/hmc/mpi_cross_chain_adapter.hpp>
#include <stan/analyze/mcmc/compute_potential_scale_reduction.hpp>
#include <stan/math/mpi/envionment.hpp>
#include <stan/callbacks/stream_logger.hpp>
#include <stan/mcmc/hmc/nuts/adapt_unit_e_nuts.hpp>
#include <boost/random/additive_combine.hpp>
#include <stan/callbacks/writer.hpp>
#include <stan/services/util/mcmc_writer.hpp>
#include <stan/services/util/generate_transitions.hpp>
#include <stan/io/dump.hpp>
#include <fstream>

#include <stan/services/sample/hmc_nuts_diag_e_adapt.hpp>
#include <stan/services/sample/hmc_nuts_dense_e_adapt.hpp>
#include <stan/io/empty_var_context.hpp>
#include <test/test-models/good/mcmc/hmc/common/gauss3D.hpp>
#include <test/unit/services/instrumented_callbacks.hpp>
#include <test/unit/services/check_adaptation.hpp>
#include <iostream>
#include <gtest/gtest.h>

using Eigen::MatrixXd;
using Eigen::Matrix;
using std::vector;
using stan::math::mpi::Session;
using stan::math::mpi::Communicator;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using boost::accumulators::accumulator_set;
using boost::accumulators::stats;
using boost::accumulators::tag::mean;
using boost::accumulators::tag::variance;

class CrossChainAdapterTest : public testing::Test {
 public:
  CrossChainAdapterTest() :
    num_chains(4),
    max_num_windows(5),
    cross_chain_window_size(3),
    comm(Session::inter_chain_comm(num_chains)),
    num_iterations(cross_chain_window_size * max_num_windows),
    cross_chain_rhat(1.1),
    cross_chain_ess(100),
    num_warmup(100)
  {}

  std::stringstream model_log;
  stan::callbacks::logger logger;
  stan::callbacks::writer init, parameter, diagnostic;
  int num_chains;     // must run with mpiexec -n with n>=4
  int max_num_windows;
  int cross_chain_window_size;
  const Communicator& comm;
  int num_iterations;
  double cross_chain_rhat;
  double cross_chain_ess;
  int num_warmup;
};

TEST_F(CrossChainAdapterTest, stepsize) { // must be run with nproc = 4
  const int n_par = 4;
  Eigen::VectorXd q = Eigen::VectorXd::Zero(n_par);

  stan::mcmc::mpi_cross_chain_adapter adapter;
  adapter.set_cross_chain_adaptation_params(num_warmup,
                                            cross_chain_window_size, num_chains,
                                            cross_chain_rhat, cross_chain_ess);  
  stan::mcmc::mpi_var_adaptation var_adapt(n_par, num_warmup, cross_chain_window_size);
  adapter.set_cross_chain_metric_adaptation(&var_adapt);

  double chain_stepsize = 0.13 + comm.rank();
  double harmonic_mean_stepsize = 0.427066;
  adapter.add_cross_chain_sample(1.1, q);
  EXPECT_FLOAT_EQ(adapter.cross_chain_stepsize(chain_stepsize), chain_stepsize);
  adapter.add_cross_chain_sample(1.2, q);
  EXPECT_FLOAT_EQ(adapter.cross_chain_stepsize(chain_stepsize), chain_stepsize);
  adapter.add_cross_chain_sample(1.2, q);
  adapter.set_cross_chain_adapted(true);
  EXPECT_FLOAT_EQ(adapter.cross_chain_stepsize(chain_stepsize), harmonic_mean_stepsize);
}

TEST_F(CrossChainAdapterTest, gather) { // must be run with nproc = 4
  const int n_par = 4;
  Eigen::VectorXd q = Eigen::VectorXd::Zero(n_par);
  q(comm.rank()) = comm.rank();

  stan::mcmc::mpi_cross_chain_adapter adapter;
  adapter.set_cross_chain_adaptation_params(num_warmup,
                                            cross_chain_window_size, num_chains,
                                            cross_chain_rhat, cross_chain_ess);  
  stan::mcmc::mpi_var_adaptation var_adapt(n_par, num_warmup, cross_chain_window_size);
  adapter.set_cross_chain_metric_adaptation(&var_adapt);

  double lp;
  // window 1
  lp = 1.3 + comm.rank();
  adapter.add_cross_chain_sample(lp, q);
  lp = 3.8 + comm.rank();
  adapter.add_cross_chain_sample(lp, q);
  lp = 0.1 + comm.rank();
  adapter.add_cross_chain_sample(lp, q);
  std::vector<double> all_chain_gather;
  adapter.cross_chain_gather(all_chain_gather);
  if (comm.rank() == 0) {
    EXPECT_EQ(all_chain_gather.size(), 20);
    EXPECT_FLOAT_EQ(all_chain_gather[0], 1.7333333333);
    EXPECT_FLOAT_EQ(all_chain_gather[1], 3.5633333333);
    EXPECT_FLOAT_EQ(all_chain_gather[2], 1.3);
    EXPECT_FLOAT_EQ(all_chain_gather[3], 3.8);
    EXPECT_FLOAT_EQ(all_chain_gather[4], 0.1);
    EXPECT_FLOAT_EQ(all_chain_gather[15], 4.7333333333);
    EXPECT_FLOAT_EQ(all_chain_gather[16], 3.5633333333);
    EXPECT_FLOAT_EQ(all_chain_gather[17], 4.3);
    EXPECT_FLOAT_EQ(all_chain_gather[18], 6.8);
    EXPECT_FLOAT_EQ(all_chain_gather[19], 3.1);
  } else {
    EXPECT_EQ(all_chain_gather.size(), 0);
  }
}

class ServicesSampleHmcNutsDiagEMassMatrix : public testing::Test {
 public:
  ServicesSampleHmcNutsDiagEMassMatrix() : model(context, &model_log) {}

  std::stringstream model_log;
  stan::callbacks::logger logger;
  stan::callbacks::writer init, parameter, diagnostic;
  stan::io::empty_var_context context;
  stan_model model;
};

TEST_F(ServicesSampleHmcNutsDiagEMassMatrix, mpi_return_checks) {
  unsigned int random_seed = 12345;
  unsigned int chain = 1;
  double init_radius = 2;
  int num_warmup = 21;
  int num_samples = 0;
  int num_thin = 1;
  bool save_warmup = false;
  int refresh = 0;
  double stepsize = 1;
  double stepsize_jitter = 0;
  int max_depth = 10;
  double delta = .8;
  double gamma = .05;
  double kappa = .75;
  double t0 = 10;
  unsigned int init_buffer = 75;
  unsigned int term_buffer = 50;
  unsigned int window = 25;

  stan::test::unit::instrumented_interrupt interrupt;
  EXPECT_EQ(interrupt.call_count(), 0);

  std::string txt
      = "inv_metric <- structure(c(0.787405, 0.884987, 1.19869),.Dim=c(3))";
  std::stringstream in(txt);
  stan::io::dump dump(in);
  stan::io::var_context& inv_metric = dump;

  // cross chain settings
  const int num_chains = 4;     // must run with mpiexec -n with n>=4
  const int max_num_windows = 5;
  const int cross_chain_window_size = 5;
  const Communicator& comm = Session::inter_chain_comm(num_chains);
  const int num_iterations = cross_chain_window_size * max_num_windows;
  const double cross_chain_rhat = 1.1;
  const double cross_chain_ess = 100;

  if (stan::math::mpi::Session::is_in_inter_chain_comm(num_chains)) {
    const Communicator& comm = Session::inter_chain_comm(num_chains);
    random_seed += comm.rank();
  }

  int return_code = stan::services::sample::hmc_nuts_diag_e_adapt(
      model, context, inv_metric, random_seed, chain, init_radius,
      num_chains, cross_chain_window_size, cross_chain_rhat, cross_chain_ess,
      num_warmup,
      num_samples, num_thin, save_warmup, refresh, stepsize, stepsize_jitter,
      max_depth, delta, gamma, kappa, t0, init_buffer, term_buffer, window,
      interrupt, logger, init, parameter, diagnostic);

  EXPECT_EQ(0, return_code);
}

class ServicesSampleHmcNutsDenseEMassMatrix : public testing::Test {
 public:
  ServicesSampleHmcNutsDenseEMassMatrix() : model(context, &model_log) {}

  std::stringstream model_log;
  stan::test::unit::instrumented_logger logger;
  stan::test::unit::instrumented_writer init, parameter, diagnostic;
  stan::io::empty_var_context context;
  stan_model model;
};

TEST_F(ServicesSampleHmcNutsDenseEMassMatrix, mpi_return_checks) {
  unsigned int random_seed = 12345;
  unsigned int chain = 1;
  double init_radius = 2;
  int num_warmup = 21;
  int num_samples = 0;
  int num_thin = 1;
  bool save_warmup = false;
  int refresh = 0;
  double stepsize = 1;
  double stepsize_jitter = 0;
  int max_depth = 10;
  double delta = .8;
  double gamma = .05;
  double kappa = .75;
  double t0 = 10;
  unsigned int init_buffer = 75;
  unsigned int term_buffer = 50;
  unsigned int window = 25;
  stan::test::unit::instrumented_interrupt interrupt;
  EXPECT_EQ(interrupt.call_count(), 0);

  std::string txt
      = "inv_metric <- structure(c("
        "0.640211, 0.156096, -0.374048, "
        "0.156096, 1.41239, -0.0412753, "
        "-0.374048, -0.0412753, 1.29567 "
        "), .Dim  = c(3,3))";
  std::stringstream in(txt);
  stan::io::dump dump(in);
  stan::io::var_context& inv_metric = dump;

  // cross chain settings
  const int num_chains = 4;     // must run with mpiexec -n with n>=4
  const int max_num_windows = 5;
  const int cross_chain_window_size = 5;
  const Communicator& comm = Session::inter_chain_comm(num_chains);
  const int num_iterations = cross_chain_window_size * max_num_windows;
  const double cross_chain_rhat = 1.1;
  const double cross_chain_ess = 100;

  if (stan::math::mpi::Session::is_in_inter_chain_comm(num_chains)) {
    const Communicator& comm = Session::inter_chain_comm(num_chains);
    random_seed += comm.rank();
  }

  int return_code = stan::services::sample::hmc_nuts_dense_e_adapt(
      model, context, inv_metric, random_seed, chain, init_radius,
      num_chains, cross_chain_window_size, cross_chain_rhat, cross_chain_ess,
      num_warmup,
      num_samples, num_thin, save_warmup, refresh, stepsize, stepsize_jitter,
      max_depth, delta, gamma, kappa, t0, init_buffer, term_buffer, window,
      interrupt, logger, init, parameter, diagnostic);
  EXPECT_EQ(0, return_code);
}

#endif
