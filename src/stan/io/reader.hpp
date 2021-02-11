#ifndef STAN_IO_READER_HPP
#define STAN_IO_READER_HPP

#include <stan/math/rev.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace stan {

namespace io {

/**
 * A stream-based reader for integer, scalar, vector, matrix
 * and array data types, with Jacobian calculations.
 *
 * The template parameter <code>T</code> represents the type of
 * scalars and the values in vectors and matrices.  The only
 * requirement on the template type <code>T</code> is that a
 * double can be copied into it, as in
 *
 * <code>T t = 0.0;</code>
 *
 * This includes <code>double</code> itself and the reverse-mode
 * algorithmic differentiation class <code>stan::math::var</code>.
 *
 * <p>For transformed values, the scalar type parameter <code>T</code>
 * must support the transforming operations, such as <code>exp(x)</code>
 * for positive-bounded variables.  It must also support equality and
 * inequality tests with <code>double</code> values.
 *
 * @tparam T Basic scalar type.
 */
template <typename T>
class reader {
 private:
  Eigen::Map<Eigen::Matrix<T, -1, 1>> data_r_;
  Eigen::Map<Eigen::Matrix<int, -1, 1>> data_i_;
  size_t pos_{0};
  size_t int_pos_{0};

  inline const T &scalar_ptr() { return data_r_.coeffRef(pos_); }

  inline const T &scalar_ptr_increment(size_t m) {
    pos_ += m;
    return data_r_.coeffRef(pos_ - m);
  }

  inline const int &int_ptr() { return data_i_.coeffRef(int_pos_); }

  inline const int &int_ptr_increment(size_t m) {
    int_pos_ += m;
    return data_i_.coeffRef(int_pos_ - m);
  }

 public:
  using matrix_t = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using vector_t = Eigen::Matrix<T, Eigen::Dynamic, 1>;
  using row_vector_t = Eigen::Matrix<T, 1, Eigen::Dynamic>;

  using map_matrix_t = Eigen::Map<const matrix_t>;
  using map_vector_t = Eigen::Map<const vector_t>;
  using map_row_vector_t = Eigen::Map<const row_vector_t>;

  using var_matrix_t = stan::math::var_value<
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>;
  using var_vector_t
      = stan::math::var_value<Eigen::Matrix<double, Eigen::Dynamic, 1>>;
  using var_row_vector_t
      = stan::math::var_value<Eigen::Matrix<double, 1, Eigen::Dynamic>>;

  /**
   * Construct a variable reader using the specified vectors
   * as the source of scalar and integer values for data.  This
   * class holds a reference to the specified data vectors.
   *
   * Attempting to read beyond the end of the data or integer
   * value sequences raises a runtime exception.
   *
   * @param data_r Sequence of scalar values.
   * @param data_i Sequence of integer values.
   */
  template <typename RVec, typename IntVec,
            require_all_vector_like_t<RVec, IntVec> * = nullptr>
  reader(RVec &data_r, IntVec &data_i)
      : data_r_(data_r.data(), data_r.size()),
        data_i_(data_i.data(), data_i.size()) {}

  /**
   * Return the number of scalars remaining to be read.
   *
   * @return Number of scalars left to read.
   */
  inline size_t available() const { return data_r_.size() - pos_; }

  /**
   * Return the number of integers remaining to be read.
   *
   * @return Number of integers left to read.
   */
  inline size_t available_i() const { return data_i_.size() - int_pos_; }

  /**
   * Return the next integer in the integer sequence.
   *
   * @return Next integer value.
   */
  inline int integer() {
    if (int_pos_ >= data_i_.size()) {
      throw std::runtime_error("no more integers to read.");
    }
    return data_i_.coeffRef(int_pos_++);
  }

  /**
   * Return the next integer in the integer sequence.
   * This form is a convenience method to make compiling
   * easier; its behavior is the same as <code>int()</code>
   *
   * @return Next integer value.
   */
  inline int integer_constrain() { return integer(); }

  /**
   * Return the next integer in the integer sequence.
   * This form is a convenience method to make compiling
   * easier; its behavior is the same as <code>integer()</code>
   *
   * @return Next integer value.
   */
  inline int integer_constrain(T & /*log_prob*/) { return integer(); }

  /**
   * Return the next scalar in the sequence.
   *
   * @return Next scalar value.
   */
  inline T scalar() {
    if (pos_ >= data_r_.size()) {
      throw std::runtime_error("no more scalars to read");
    }
    return data_r_.coeffRef(pos_++);
  }

  /**
   * Return the next scalar.  For arbitrary scalars,
   * constraint is a no-op.
   *
   * @return Next scalar.
   */
  inline T scalar_constrain() { return scalar(); }

  /**
   * Return the next scalar in the sequence, incrementing
   * the specified reference with the log absolute Jacobian determinant.
   *
   * <p>With no transformation, the Jacobian increment is a no-op.
   *
   * <p>See <code>scalar_constrain()</code>.
   *
   * log_prob Reference to log probability variable to increment.
   * @return Next scalar.
   */
  template <bool Jacobian = true>
  inline T scalar_constrain(T & /*log_prob*/) { return scalar(); }

  /**
   * Return a standard library vector of the specified
   * dimensionality made up of the next scalars.
   *
   * @param m Size of vector.
   * @return Vector made up of the next scalars.
   */
  inline std::vector<T> std_vector(size_t m) {
    if (m == 0)
      return std::vector<T>();
    const auto *start_pos = &this->data_r_.coeffRef(this->pos_);
    const auto *end_pos = &this->data_r_.coeffRef(this->pos_ + m);
    this->pos_ += m;
    return std::vector<T>(start_pos, end_pos);
  }

  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  inline auto vector(size_t m) {
    if (m == 0)
      return map_vector_t(nullptr, m);
    return map_vector_t(&scalar_ptr_increment(m), m);
  }

  /**
   * Return a `var_value` with inner type column vector with specified
   * dimensionality made up of the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  template <typename T_ = T, require_st_var<T_> * = nullptr>
  inline var_vector_t var_vector(size_t m) {
    if (m == 0)
      return var_vector_t(Eigen::VectorXd(0));
    return stan::math::to_var_value(map_vector_t(&scalar_ptr_increment(m), m));
  }

  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  template <typename T_ = T, require_st_arithmetic<T_> * = nullptr>
  inline vector_t var_vector(size_t m) {
    return this->vector(m);
  }

  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.  The constraint is a no-op.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  inline auto vector_constrain(size_t m) {
    return this->vector(m);
  }
  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.  The constraint and hence Jacobian are no-ops.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true>
  inline auto vector_constrain(size_t m, T & /*lp*/) {
    return this->vector(m);
  }

  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.  The constraint and hence Jacobian are no-ops.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_arithmetic<T_>* = nullptr>
  inline auto var_vector_constrain(size_t m, T & /*lp*/) {
    return this->vector(m);
  }

  /**
   * Return a column vector of specified dimensionality made up of
   * the next scalars.  The constraint and hence Jacobian are no-ops.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_var<T_>* = nullptr>
  inline auto var_vector_constrain(size_t m, T & /*lp*/) {
    return this->var_vector(m);
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  inline auto row_vector(size_t m) {
    if (m == 0)
      return map_row_vector_t(nullptr, m);
    return map_row_vector_t(&scalar_ptr_increment(m), m);
  }

  /**
   * Return a `var_value` with inner type as a row vector with specified
   * dimensionality made up of the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  template <typename T_ = T, require_st_var<T_> * = nullptr>
  inline var_row_vector_t var_row_vector(size_t m) {
    if (m == 0)
      return var_row_vector_t(Eigen::RowVectorXd(0));
    return stan::math::to_var_value(
        map_row_vector_t(&scalar_ptr_increment(m), m));
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  template <typename T_ = T, require_st_arithmetic<T_> * = nullptr>
  inline row_vector_t var_row_vector(size_t m) {
    return this->row_vector(m);
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.  The constraint is a no-op.
   *
   * @param m Number of rows in the vector to read.
   * @return Column vector made up of the next scalars.
   */
  inline auto row_vector_constrain(size_t m) {
    return this->row_vector(m);
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.  The constraint is a no-op, so the log
   * probability is not incremented.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true>
  inline auto row_vector_constrain(size_t m, T & /*lp*/) {
    return this->row_vector(m);
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.  The constraint is a no-op, so the log
   * probability is not incremented.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_var<T_>* = nullptr>
  inline auto var_row_vector_constrain(size_t m, T & /*lp*/) {
    return this->var_row_vector(m);
  }

  /**
   * Return a row vector of specified dimensionality made up of
   * the next scalars.  The constraint is a no-op, so the log
   * probability is not incremented.
   *
   * @param m Number of rows in the vector to read.
   * lp Log probability to increment.
   * @return Column vector made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_arithmetic<T_>* = nullptr>
  inline auto var_row_vector_constrain(size_t m, T & /*lp*/) {
    return this->var_row_vector(m);
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.
   *
   * Row-major reading means that if a matrix of <code>m=2</code>
   * rows and <code>n=3</code> columns is read and the next
   * scalar values are <code>1,2,3,4,5,6</code>, the result is
   *
   * <pre>
   * a = 1 4
   *     2 5
   *     3 6</pre>
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * @return Eigen::Matrix made up of the next scalars.
   */
  inline auto matrix(size_t m, size_t n) {
    if (m == 0 || n == 0) {
      return map_matrix_t(nullptr, m, n);
    } else {
      return map_matrix_t(&scalar_ptr_increment(m * n), m, n);
    }
  }

  /**
   * Return a `var_value` with inner type matrix with the specified
   * dimensionality made up of  the next scalars arranged in column-major order.
   *
   * Row-major reading means that if a matrix of <code>m=2</code>
   * rows and <code>n=3</code> columns is read and the next
   * scalar values are <code>1,2,3,4,5,6</code>, the result is
   *
   * <pre>
   * a = 1 4
   *     2 5
   *     3 6</pre>
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * @return Eigen::Matrix made up of the next scalars.
   */
  template <typename T_ = T, require_st_var<T_> * = nullptr>
  inline var_matrix_t var_matrix(size_t m, size_t n) {
    if (m == 0 || n == 0) {
      return var_matrix_t(Eigen::MatrixXd(0, 0));
    } else {
      return stan::math::to_var_value(
          map_matrix_t(&scalar_ptr_increment(m * n), m, n));
    }
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.
   *
   * Row-major reading means that if a matrix of <code>m=2</code>
   * rows and <code>n=3</code> columns is read and the next
   * scalar values are <code>1,2,3,4,5,6</code>, the result is
   *
   * <pre>
   * a = 1 4
   *     2 5
   *     3 6</pre>
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * @return Eigen::Matrix made up of the next scalars.
   */
  template <typename T_ = T, require_st_arithmetic<T_> * = nullptr>
  inline matrix_t var_matrix(size_t m, size_t n) {
    return this->matrix(m, n);
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.  The
   * constraint is a no-op.  See <code>matrix(size_t,
   * size_t)</code> for more information.
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * @return Matrix made up of the next scalars.
   */
  inline auto matrix_constrain(size_t m, size_t n) {
    return this->matrix(m, n);
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.  The
   * constraint is a no-op, hence the log probability is not
   * incremented.  See <code>matrix(size_t, size_t)</code>
   * for more information.
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * lp Log probability to increment.
   * @return Matrix made up of the next scalars.
   */
  template <bool Jacobian = true>
  inline auto matrix_constrain(size_t m, size_t n, T & /*lp*/) {
    return this->matrix(m, n);
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.  The
   * constraint is a no-op, hence the log probability is not
   * incremented.  See <code>matrix(size_t, size_t)</code>
   * for more information.
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * lp Log probability to increment.
   * @return Matrix made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_arithmetic<T_>* = nullptr>
  inline auto var_matrix_constrain(size_t m, size_t n, T & /*lp*/) {
    return this->matrix(m, n);
  }

  /**
   * Return a matrix of the specified dimensionality made up of
   * the next scalars arranged in column-major order.  The
   * constraint is a no-op, hence the log probability is not
   * incremented.  See <code>matrix(size_t, size_t)</code>
   * for more information.
   *
   * @param m Number of rows.
   * @param n Number of columns.
   * lp Log probability to increment.
   * @return Matrix made up of the next scalars.
   */
  template <bool Jacobian = true, typename T_ = T, require_st_var<T_>* = nullptr>
  inline auto var_matrix_constrain(size_t m, size_t n, T & /*lp*/) {
    return this->var_matrix(m, n);
  }

  /**
   * Return the next integer, checking that it is greater than
   * or equal to the specified lower bound.
   *
   * @param lb Lower bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * greater than or equal to the lower bound.
   */
  inline int integer_lb(int lb) {
    int i = integer();
    if (!(i >= lb)) {
      throw std::runtime_error("required value greater than or equal to lb");
    }
    return i;
  }
  /**
   * Return the next integer, checking that it is greater than
   * or equal to the specified lower bound.
   *
   * @param lb Lower bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * greater than or equal to the lower bound.
   */
  inline int integer_lb_constrain(int lb) { return integer_lb(lb); }
  /**
   * Return the next integer, checking that it is greater than
   * or equal to the specified lower bound.
   *
   * @param lb Lower bound.
   * lp Log probability (ignored because no Jacobian)
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * greater than or equal to the lower bound.
   */
  template <bool Jacobian = true>
  inline int integer_lb_constrain(int lb, T & /*lp*/) { return integer_lb(lb); }

  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.
   *
   * @param ub Upper bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  inline int integer_ub(int ub) {
    int i = integer();
    if (!(i <= ub)) {
      throw std::runtime_error("required value less than or equal to ub");
    }
    return i;
  }
  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.
   *
   * @param ub Upper bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  inline int integer_ub_constrain(int ub) { return integer_ub(ub); }
  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.
   *
   * @param ub Upper bound.
   * lp Log probability (ignored because no Jacobian)
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  template <bool Jacobian = true>
  inline int integer_ub_constrain(int ub, T & /*lp*/) { return integer_ub(ub); }

  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.  Even if the upper
   * bounds and lower bounds are not consistent, the next integer
   * value will be consumed.
   *
   * @param lb Lower bound.
   * @param ub Upper bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  inline int integer_lub(int lb, int ub) {
    // read first to make position deterministic [arbitrary choice]
    int i = integer();
    if (lb > ub) {
      throw std::runtime_error("lower bound must be less than or equal to ub");
    }
    if (!(i >= lb)) {
      throw std::runtime_error("required value greater than or equal to lb");
    }
    if (!(i <= ub)) {
      throw std::runtime_error("required value less than or equal to ub");
    }
    return i;
  }
  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.
   *
   * @param lb Lower bound.
   * @param ub Upper bound.
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  inline int integer_lub_constrain(int lb, int ub) {
    return integer_lub(lb, ub);
  }
  /**
   * Return the next integer, checking that it is less than
   * or equal to the specified upper bound.
   *
   * @param lb Lower bound.
   * @param ub Upper bound.
   * lp Log probability (ignored because no Jacobian)
   * @return Next integer read.
   * @throw std::runtime_error If the next integer read is not
   * less than or equal to the upper bound.
   */
  template <bool Jacobian = true>
  inline int integer_lub_constrain(int lb, int ub, T & /*lp*/) {
    return integer_lub(lb, ub);
  }

  /**
   * Return the next scalar, checking that it is
   * positive.
   *
   * <p>See <code>stan::math::check_positive(T)</code>.
   *
   * @return Next positive scalar.
   * @throw std::runtime_error if x is not positive
   */
  inline T scalar_pos() {
    T x(scalar());
    stan::math::check_positive("stan::io::scalar_pos", "Constrained scalar", x);
    return x;
  }

  /**
   * Return the next scalar, transformed to be positive.
   *
   * <p>See <code>stan::math::positive_constrain(T)</code>.
   *
   * @return The next scalar transformed to be positive.
   */
  inline T scalar_pos_constrain() {
    return stan::math::positive_constrain(scalar());
  }

  /**
   * Return the next scalar transformed to be positive,
   * incrementing the specified reference with the log absolute
   * determinant of the Jacobian.
   *
   * <p>See <code>stan::math::positive_constrain(T,T&)</code>.
   *
   * @param lp Reference to log probability variable to increment.
   * @return The next scalar transformed to be positive.
   */
  template <bool Jacobian = true>
  inline T scalar_pos_constrain(T &lp) {
    if (Jacobian) {
      return stan::math::positive_constrain(scalar(), lp);
    } else {
      return stan::math::positive_constrain(scalar());
    }
  }

  /**
   * Return the next scalar, checking that it is
   * greater than or equal to the specified lower bound.
   *
   * <p>See <code>stan::math::check_greater_or_equal(T,double)</code>.
   *
   * @param lb Lower bound.
   * @return Next scalar value.
   * @tparam TL Type of lower bound.
   * @throw std::runtime_error if the scalar is less than the
   *    specified lower bound
   */
  template <typename TL>
  inline T scalar_lb(const TL lb) {
    T x(scalar());
    stan::math::check_greater_or_equal("stan::io::scalar_lb",
                                       "Constrained scalar", x, lb);
    return x;
  }

  /**
   * Return the next scalar transformed to have the
   * specified lower bound.
   *
   * <p>See <code>stan::math::lb_constrain(T,double)</code>.
   *
   * @tparam TL Type of lower bound.
   * @param lb Lower bound on values.
   * @return Next scalar transformed to have the specified
   * lower bound.
   */
  template <typename TL>
  inline T scalar_lb_constrain(const TL lb) {
    return stan::math::lb_constrain(scalar(), lb);
  }

  /**
   * Return the next scalar transformed to have the specified
   * lower bound, incrementing the specified reference with the
   * log of the absolute Jacobian determinant of the transform.
   *
   * <p>See <code>stan::math::lb_constrain(T,double,T&)</code>.
   *
   * @tparam TL Type of lower bound.
   * @param lb Lower bound on result.
   * @param lp Reference to log probability variable to increment.
   */
  template <bool Jacobian = true, typename TL>
  inline T scalar_lb_constrain(const TL lb, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(scalar(), lb, lp);
    } else {
      return stan::math::lb_constrain(scalar(), lb);
    }
  }

  /**
   * Return the next scalar, checking that it is
   * less than or equal to the specified upper bound.
   *
   * <p>See <code>stan::math::check_less_or_equal(T,double)</code>.
   *
   * @tparam TU Type of upper bound.
   * @param ub Upper bound.
   * @return Next scalar value.
   * @throw std::runtime_error if the scalar is greater than the
   *    specified upper bound
   */
  template <typename TU>
  inline T scalar_ub(TU ub) {
    T x(scalar());
    stan::math::check_less_or_equal("stan::io::scalar_ub", "Constrained scalar",
                                    x, ub);
    return x;
  }

  /**
   * Return the next scalar transformed to have the
   * specified upper bound.
   *
   * <p>See <code>stan::math::ub_constrain(T,double)</code>.
   *
   * @tparam TU Type of upper bound.
   * @param ub Upper bound on values.
   * @return Next scalar transformed to have the specified
   * upper bound.
   */
  template <typename TU>
  inline T scalar_ub_constrain(const TU ub) {
    return stan::math::ub_constrain(scalar(), ub);
  }

  /**
   * Return the next scalar transformed to have the specified
   * upper bound, incrementing the specified reference with the
   * log of the absolute Jacobian determinant of the transform.
   *
   * <p>See <code>stan::math::ub_constrain(T,double,T&)</code>.
   *
   * @tparam TU Type of upper bound.
   * @param ub Upper bound on result.
   * @param lp Reference to log probability variable to increment.
   */
  template <bool Jacobian = true, typename TU>
  inline T scalar_ub_constrain(const TU ub, T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(scalar(), ub, lp);
    } else {
      return stan::math::ub_constrain(scalar(), ub);
    }
  }

  /**
   * Return the next scalar, checking that it is between
   * the specified lower and upper bound.
   *
   * <p>See <code>stan::math::check_bounded(T, double, double)</code>.
   *
   * @tparam TL Type of lower bound.
   * @tparam TU Type of upper bound.
   * @param lb Lower bound.
   * @param ub Upper bound.
   * @return Next scalar value.
   * @throw std::runtime_error if the scalar is not between the specified
   *    lower and upper bounds.
   */
  template <typename TL, typename TU>
  inline T scalar_lub(const TL lb, const TU ub) {
    T x(scalar());
    stan::math::check_bounded<T, TL, TU>("stan::io::scalar_lub",
                                         "Constrained scalar", x, lb, ub);
    return x;
  }

  /**
   * Return the next scalar transformed to be between
   * the specified lower and upper bounds.
   *
   * <p>See <code>stan::math::lub_constrain(T, double, double)</code>.
   *
   * @tparam TL Type of lower bound.
   * @tparam TU Type of upper bound.
   * @param lb Lower bound.
   * @param ub Upper bound.
   * @return Next scalar transformed to fall between the specified
   * bounds.
   */
  template <typename TL, typename TU>
  inline T scalar_lub_constrain(const TL lb, const TU ub) {
    return stan::math::lub_constrain(scalar(), lb, ub);
  }

  /**
   * Return the next scalar transformed to be between the
   * the specified lower and upper bounds.
   *
   * <p>See <code>stan::math::lub_constrain(T, double, double, T&)</code>.
   *
   * @param lb Lower bound.
   * @param ub Upper bound.
   * @param lp Reference to log probability variable to increment.
   * @tparam T Type of scalar.
   * @tparam TL Type of lower bound.
   * @tparam TU Type of upper bound.
   */
  template <bool Jacobian = true, typename TL, typename TU>
  inline T scalar_lub_constrain(TL lb, TU ub, T &lp) {
    if (Jacobian) {
      return stan::math::lub_constrain(scalar(), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(scalar(), lb, ub);
    }
  }

  /**
   * Return the next scalar.
   *
   * @tparam TL type of offset
   * @tparam TS type of multiplier
   * @param offset offset
   * @param multiplier multiplier
   * @return next scalar value
   */
  template <typename TL, typename TS>
  inline T scalar_offset_multiplier(const TL offset, const TS multiplier) {
    return scalar();
  }

  /**
   * Return the next scalar transformed to have the specified offset and
   * multiplier.
   *
   * <p>See <code>stan::math::offset_multiplier_constrain(T, double,
   * double)</code>.
   *
   * @tparam TL Type of offset.
   * @tparam TS Type of multiplier.
   * @param offset Offset.
   * @param multiplier Multiplier.
   * @return Next scalar transformed to fall between the specified
   * bounds.
   */
  template <typename TL, typename TS>
  inline T scalar_offset_multiplier_constrain(const TL offset,
                                              const TS multiplier) {
    return stan::math::offset_multiplier_constrain(scalar(), offset,
                                                   multiplier);
  }

  /**
   * Return the next scalar transformed to have the specified offset and
   * multiplier.
   *
   * <p>See <code>stan::math::offset_multiplier_constrain(T, double, double,
   * T&)</code>.
   *
   * @param offset Offset.
   * @param multiplier Multiplier.
   * @param lp Reference to log probability variable to increment.
   * @tparam T Type of scalar.
   * @tparam TL Type of offset.
   * @tparam TS Type of multiplier.
   */
  template <bool Jacobian = true, typename TL, typename TS>
  inline T scalar_offset_multiplier_constrain(TL offset, TS multiplier, T &lp) {
    if (Jacobian) {
      return stan::math::offset_multiplier_constrain(scalar(), offset, multiplier,
                                                     lp);
    } else {
      return stan::math::offset_multiplier_constrain(scalar(), offset, multiplier);
    }
  }

  /**
   * Return the next scalar, checking that it is a valid value for
   * a probability, between 0 (inclusive) and 1 (inclusive).
   *
   * <p>See <code>stan::math::check_bounded(T)</code>.
   *
   * @return Next probability value.
   */
  inline T prob() {
    T x(scalar());
    stan::math::check_bounded<T, double, double>(
        "stan::io::prob", "Constrained probability", x, 0, 1);
    return x;
  }

  /**
   * Return the next scalar transformed to be a probability
   * between 0 and 1.
   *
   * <p>See <code>stan::math::prob_constrain(T)</code>.
   *
   * @return The next scalar transformed to a probability.
   */
  inline T prob_constrain() { return stan::math::prob_constrain(scalar()); }

  /**
   * Return the next scalar transformed to be a probability
   * between 0 and 1, incrementing the specified reference with
   * the log of the absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::prob_constrain(T)</code>.
   *
   * @param lp Reference to log probability variable to increment.
   * @return The next scalar transformed to a probability.
   */
  template <bool Jacobian = true>
  inline T prob_constrain(T &lp) {
    if (Jacobian) {
      return stan::math::prob_constrain(scalar(), lp);
    } else {
      return stan::math::prob_constrain(scalar());
    }
  }

  /**
   * Return the next scalar, checking that it is a valid
   * value for a correlation, between -1 (inclusive) and
   * 1 (inclusive).
   *
   * <p>See <code>stan::math::check_bounded(T)</code>.
   *
   * @return Next correlation value.
   * @throw std::runtime_error if the value is not valid
   *   for a correlation
   */
  inline T corr() {
    T x(scalar());
    stan::math::check_bounded<T, double, double>("stan::io::corr",
                                                 "Correlation value", x, -1, 1);
    return x;
  }

  /**
   * Return the next scalar transformed to be a correlation
   * between -1 and 1.
   *
   * <p>See <code>stan::math::corr_constrain(T)</code>.
   *
   * @return The next scalar transformed to a correlation.
   */
  inline T corr_constrain() { return stan::math::corr_constrain(scalar()); }

  /**
   * Return the next scalar transformed to be a (partial)
   * correlation between -1 and 1, incrementing the specified
   * reference with the log of the absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::corr_constrain(T,T&)</code>.
   *
   * @param lp The reference to the variable holding the log
   * probability to increment.
   * @return The next scalar transformed to a correlation.
   */
  template <bool Jacobian = true>
  inline T corr_constrain(T &lp) {
    if (Jacobian) {
      return stan::math::corr_constrain(scalar(), lp);
    } else {
      return stan::math::corr_constrain(scalar());
    }
  }

  /**
   * Return a unit_vector of the specified size made up of the
   * next scalars.
   *
   * <p>See <code>stan::math::check_unit_vector</code>.
   *
   * @param k Size of returned unit_vector
   * @return unit_vector read from the specified size number of scalars
   * @throw std::runtime_error if the next k values is not a unit_vector
   * @throw std::invalid_argument if k is zero
   */
  inline map_vector_t unit_vector(size_t k) {
    if (k == 0) {
      std::string msg = "io::unit_vector: unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    map_vector_t theta(vector(k));
    stan::math::check_unit_vector("stan::io::unit_vector", "Constrained vector",
                                  theta);
    return theta;
  }

  /**
   * Return a unit_vector of the specified size made up of the
   * next scalars.
   *
   * <p>See <code>stan::math::check_unit_vector</code>.
   *
   * @param k Size of returned unit_vector
   * @return unit_vector read from the specified size number of scalars
   * @throw std::runtime_error if the next k values is not a unit_vector
   * @throw std::invalid_argument if k is zero
   */
  inline var_vector_t var_unit_vector(size_t k) {
    using stan::math::value_of;
    if (k == 0) {
      std::string msg = "io::unit_vector: unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    var_vector_t theta(var_vector(k));
    stan::math::check_unit_vector("stan::io::unit_vector", "Constrained vector",
                                  value_of(theta));
    return theta;
  }

  /**
   * Return the next unit_vector transformed vector of the specified
   * length.  This operation consumes one less than the specified
   * length number of scalars.
   *
   * <p>See <code>stan::math::unit_vector_constrain(Eigen::Matrix)</code>.
   *
   * @param k Number of dimensions in resulting unit_vector.
   * @return unit_vector derived from next <code>k</code> scalars.
   * @throw std::invalid_argument if k is zero
   */
  inline auto unit_vector_constrain(size_t k) {
    if (k == 0) {
      std::string msg
          = "io::unit_vector_constrain:"
            " unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    return stan::math::unit_vector_constrain(vector(k));
  }

  /**
   * Return the next unit_vector of the specified size (using one fewer
   * unconstrained scalars), incrementing the specified reference with the
   * log absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::unit_vector_constrain(Eigen::Matrix,T&)</code>.
   *
   * @param k Size of unit_vector.
   * @param lp Log probability to increment with log absolute
   * Jacobian determinant.
   * @return The next unit_vector of the specified size.
   * @throw std::invalid_argument if k is zero
   */
  template <bool Jacobian = true>
  inline vector_t unit_vector_constrain(size_t k, T &lp) {
    if (k == 0) {
      std::string msg
          = "io::unit_vector_constrain:"
            " unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    if (Jacobian) {
      return stan::math::unit_vector_constrain(vector(k), lp);
    } else {
      return stan::math::unit_vector_constrain(vector(k));
    }
  }

  /**
   * Return the next unit_vector of the specified size (using one fewer
   * unconstrained scalars), incrementing the specified reference with the
   * log absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::unit_vector_constrain(Eigen::Matrix,T&)</code>.
   *
   * @param k Size of unit_vector.
   * @param lp Log probability to increment with log absolute
   * Jacobian determinant.
   * @return The next unit_vector of the specified size.
   * @throw std::invalid_argument if k is zero
   */
  template <bool Jacobian = true>
  inline var_vector_t var_unit_vector_constrain(size_t k, T &lp) {
    if (k == 0) {
      std::string msg
          = "io::unit_vector_constrain:"
            " unit vectors cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    if (Jacobian) {
      return stan::math::unit_vector_constrain(var_vector(k), lp);
    } else {
      return stan::math::unit_vector_constrain(var_vector(k));
    }
  }

  /**
   * Return a simplex of the specified size made up of the
   * next scalars.
   *
   * <p>See <code>stan::math::check_simplex</code>.
   *
   * @param k Size of returned simplex.
   * @return Simplex read from the specified size number of scalars.
   * @throw std::runtime_error if the k values is not a simplex.
   * @throw std::invalid_argument if k is zero
   */
  inline map_vector_t simplex(size_t k) {
    if (k == 0) {
      std::string msg = "io::simplex: simplexes cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    map_vector_t theta(vector(k));
    stan::math::check_simplex("stan::io::simplex", "Constrained vector", theta);
    return theta;
  }

  /**
   * Return a simplex of the specified size made up of the
   * next scalars.
   *
   * <p>See <code>stan::math::check_simplex</code>.
   *
   * @param k Size of returned simplex.
   * @return Simplex read from the specified size number of scalars.
   * @throw std::runtime_error if the k values is not a simplex.
   * @throw std::invalid_argument if k is zero
   */
  inline var_vector_t var_simplex(size_t k) {
    using stan::math::value_of;
    if (k == 0) {
      std::string msg = "io::simplex: simplexes cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    var_vector_t theta(var_vector(k));
    stan::math::check_simplex("stan::io::simplex", "Constrained vector", value_of(theta));
    return theta;
  }

  /**
   * Return the next simplex transformed vector of the specified
   * length.  This operation consumes one less than the specified
   * length number of scalars.
   *
   * <p>See <code>stan::math::simplex_constrain(Eigen::Matrix)</code>.
   *
   * @param k number of dimensions in resulting simplex
   * @return simplex derived from next `k - 1` scalars
   * @throws std::invalid_argument if number of dimensions (`k`) is zero
   */
  inline auto simplex_constrain(size_t k) {
    if (k == 0) {
      std::string msg = "io::simplex_constrain: simplexes cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    return stan::math::simplex_constrain(vector(k - 1));
  }

  /**
   * Return the next simplex of the specified size (using one fewer
   * unconstrained scalars), incrementing the specified reference with the
   * log absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::simplex_constrain(Eigen::Matrix,T&)</code>.
   *
   * @param k Size of simplex.
   * @param lp Log probability to increment with log absolute
   * Jacobian determinant.
   * @return The next simplex of the specified size.
   * @throws std::invalid_argument if number of dimensions (`k`) is zero
   */
  template <bool Jacobian = true>
  inline vector_t simplex_constrain(size_t k, T &lp) {
    if (k == 0) {
      std::string msg = "io::simplex_constrain: simplexes cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    if (Jacobian) {
      return stan::math::simplex_constrain(vector(k - 1), lp);
    } else {
      return stan::math::simplex_constrain(vector(k - 1));
    }
  }

  /**
   * Return the next simplex of the specified size (using one fewer
   * unconstrained scalars), incrementing the specified reference with the
   * log absolute Jacobian determinant.
   *
   * <p>See <code>stan::math::simplex_constrain(Eigen::Matrix,T&)</code>.
   *
   * @param k Size of simplex.
   * @param lp Log probability to increment with log absolute
   * Jacobian determinant.
   * @return The next simplex of the specified size.
   * @throws std::invalid_argument if number of dimensions (`k`) is zero
   */
  template <bool Jacobian = true>
  inline var_vector_t var_simplex_constrain(size_t k, T &lp) {
    if (k == 0) {
      std::string msg = "io::simplex_constrain: simplexes cannot be size 0.";
      throw std::invalid_argument(msg);
    }
    if (Jacobian) {
      return stan::math::simplex_constrain(var_vector(k - 1), lp);
    } else {
      return stan::math::simplex_constrain(var_vector(k - 1));
    }
  }

  /**
   * Return the next vector of specified size containing
   * values in ascending order.
   *
   * <p>See <code>stan::math::check_ordered(T)</code> for
   * behavior on failure.
   *
   * @param k Size of returned vector.
   * @return Vector of positive values in ascending order.
   */
  inline map_vector_t ordered(size_t k) {
    map_vector_t x(vector(k));
    stan::math::check_ordered("stan::io::ordered", "Constrained vector", x);
    return x;
  }

  /**
   * Return the next vector of specified size containing
   * values in ascending order.
   *
   * <p>See <code>stan::math::check_ordered(T)</code> for
   * behavior on failure.
   *
   * @param k Size of returned vector.
   * @return Vector of positive values in ascending order.
   */
  inline var_vector_t var_ordered(size_t k) {
    using stan::math::value_of;
    var_vector_t x(var_vector(k));
    stan::math::check_ordered("stan::io::ordered", "Constrained vector", value_of(x));
    return x;
  }

  /**
   * Return the next ordered vector of the specified length.
   *
   * <p>See <code>stan::math::ordered_constrain(Matrix)</code>.
   *
   * @param k Length of returned vector.
   * @return Next ordered vector of the specified
   * length.
   */
  inline auto ordered_constrain(size_t k) {
    return stan::math::ordered_constrain(vector(k));
  }

  /**
   * Return the next ordered vector of the specified
   * size, incrementing the specified reference with the log
   * absolute Jacobian of the determinant.
   *
   * <p>See <code>stan::math::ordered_constrain(Matrix,T&)</code>.
   *
   * @param k Size of vector.
   * @param lp Log probability reference to increment.
   * @return Next ordered vector of the specified size.
   */
  template <bool Jacobian = true>
  inline vector_t ordered_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::ordered_constrain(vector(k), lp);
    } else {
      return stan::math::ordered_constrain(vector(k));
    }
  }

  /**
   * Return the next ordered vector of the specified
   * size, incrementing the specified reference with the log
   * absolute Jacobian of the determinant.
   *
   * <p>See <code>stan::math::ordered_constrain(Matrix,T&)</code>.
   *
   * @param k Size of vector.
   * @param lp Log probability reference to increment.
   * @return Next ordered vector of the specified size.
   */
  template <bool Jacobian = true>
  inline var_vector_t var_ordered_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::ordered_constrain(var_vector(k), lp);
    } else {
      return stan::math::ordered_constrain(var_vector(k));
    }
  }

  /**
   * Return the next vector of specified size containing
   * positive values in ascending order.
   *
   * <p>See <code>stan::math::check_positive_ordered(T)</code> for
   * behavior on failure.
   *
   * @param k Size of returned vector.
   * @return Vector of positive values in ascending order.
   */
  inline map_vector_t positive_ordered(size_t k) {
    map_vector_t x(vector(k));
    stan::math::check_positive_ordered("stan::io::positive_ordered",
                                       "Constrained vector", x);
    return x;
  }

  /**
   * Return the next vector of specified size containing
   * positive values in ascending order.
   *
   * <p>See <code>stan::math::check_positive_ordered(T)</code> for
   * behavior on failure.
   *
   * @param k Size of returned vector.
   * @return Vector of positive values in ascending order.
   */
  inline var_vector_t var_positive_ordered(size_t k) {
    using stan::math::value_of;
    var_vector_t x(var_vector(k));
    stan::math::check_positive_ordered("stan::io::positive_ordered",
                                       "Constrained vector", value_of(x));
    return x;
  }


  /**
   * Return the next positive ordered vector of the specified length.
   *
   * <p>See <code>stan::math::positive_ordered_constrain(Matrix)</code>.
   *
   * @param k Length of returned vector.
   * @return Next positive_ordered vector of the specified
   * length.
   */
  inline auto positive_ordered_constrain(size_t k) {
    return stan::math::positive_ordered_constrain(vector(k));
  }


  /**
   * Return the next positive ordered vector of the specified length.
   *
   * <p>See <code>stan::math::positive_ordered_constrain(Matrix)</code>.
   *
   * @param k Length of returned vector.
   * @return Next positive_ordered vector of the specified
   * length.
   */
  inline auto var_positive_ordered_constrain(size_t k) {
    return stan::math::positive_ordered_constrain(var_vector(k));
  }

  /**
   * Return the next positive_ordered vector of the specified
   * size, incrementing the specified reference with the log
   * absolute Jacobian of the determinant.
   *
   * <p>See <code>stan::math::positive_ordered_constrain(Matrix,T&)</code>.
   *
   * @param k Size of vector.
   * @param lp Log probability reference to increment.
   * @return Next positive_ordered vector of the specified size.
   */
  template <bool Jacobian = true>
  inline vector_t positive_ordered_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::positive_ordered_constrain(vector(k), lp);
    } else {
      return stan::math::positive_ordered_constrain(vector(k));
    }
  }

  /**
   * Return the next positive_ordered vector of the specified
   * size, incrementing the specified reference with the log
   * absolute Jacobian of the determinant.
   *
   * <p>See <code>stan::math::positive_ordered_constrain(Matrix,T&)</code>.
   *
   * @param k Size of vector.
   * @param lp Log probability reference to increment.
   * @return Next positive_ordered vector of the specified size.
   */
  template <bool Jacobian = true>
  inline var_vector_t var_positive_ordered_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::positive_ordered_constrain(var_vector(k), lp);
    } else {
      return stan::math::positive_ordered_constrain(var_vector(k));
    }
  }

  /**
   * Return the next Cholesky factor with the specified
   * dimensionality, reading it directly without transforms.
   *
   * @param M Rows of Cholesky factor
   * @param N Columns of Cholesky factor
   * @return Next Cholesky factor.
   * @throw std::domain_error if the matrix is not a valid
   * Cholesky factor.
   */
  inline map_matrix_t cholesky_factor_cov(size_t M, size_t N) {
    map_matrix_t y(matrix(M, N));
    stan::math::check_cholesky_factor("stan::io::cholesky_factor_cov",
                                      "Constrained matrix", y);
    return y;
  }

  /**
   * Return the next Cholesky factor with the specified
   * dimensionality, reading it directly without transforms.
   *
   * @param M Rows of Cholesky factor
   * @param N Columns of Cholesky factor
   * @return Next Cholesky factor.
   * @throw std::domain_error if the matrix is not a valid
   * Cholesky factor.
   */
  inline var_matrix_t var_cholesky_factor_cov(size_t M, size_t N) {
    var_matrix_t y(var_matrix(M, N));
    stan::math::check_cholesky_factor("stan::io::cholesky_factor_cov",
                                      "Constrained matrix", value_of(y));
    return y;
  }

  /**
   * Return the next Cholesky factor with the specified
   * dimensionality, reading from an unconstrained vector of the
   * appropriate size.
   *
   * @param M Rows of Cholesky factor
   * @param N Columns of Cholesky factor
   * @return Next Cholesky factor.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor.
   */
  inline auto cholesky_factor_cov_constrain(size_t M, size_t N) {
    return stan::math::cholesky_factor_constrain(
        vector((N * (N + 1)) / 2 + (M - N) * N), M, N);
  }

  /**
   * Return the next Cholesky factor with the specified
   * dimensionality, reading from an unconstrained vector of the
   * appropriate size, and increment the log probability reference
   * with the log Jacobian adjustment for the transform.
   *
   * @param M Rows of Cholesky factor
   * @param N Columns of Cholesky factor
   * @param[in,out] lp log probability
   * @return Next Cholesky factor.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor.
   */
  template <bool Jacobian = true>
  inline matrix_t cholesky_factor_cov_constrain(size_t M, size_t N, T &lp) {
    if (Jacobian) {
      return stan::math::cholesky_factor_constrain(
          vector((N * (N + 1)) / 2 + (M - N) * N), M, N, lp);
    } else {
      return stan::math::cholesky_factor_constrain(
          vector((N * (N + 1)) / 2 + (M - N) * N), M, N);
    }
  }

  /**
   * Return the next Cholesky factor with the specified
   * dimensionality, reading from an unconstrained vector of the
   * appropriate size, and increment the log probability reference
   * with the log Jacobian adjustment for the transform.
   *
   * @param M Rows of Cholesky factor
   * @param N Columns of Cholesky factor
   * @param[in,out] lp log probability
   * @return Next Cholesky factor.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor.
   */
  template <bool Jacobian = true>
  inline var_matrix_t var_cholesky_factor_cov_constrain(size_t M, size_t N, T &lp) {
    if (Jacobian) {
      return stan::math::cholesky_factor_constrain(
          var_vector((N * (N + 1)) / 2 + (M - N) * N), M, N, lp);
    } else {
      return stan::math::cholesky_factor_constrain(
          var_vector((N * (N + 1)) / 2 + (M - N) * N), M, N);
    }
  }

  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading it directly without
   * transforms.
   *
   * @param K Rows and columns of Cholesky factor
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   * Cholesky factor for a correlation matrix.
   */
  inline map_matrix_t cholesky_factor_corr(size_t K) {
    using stan::math::check_cholesky_factor_corr;
    map_matrix_t y(matrix(K, K));
    check_cholesky_factor_corr("stan::io::cholesky_factor_corr",
                               "Constrained matrix", y);
    return y;
  }

  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading it directly without
   * transforms.
   *
   * @param K Rows and columns of Cholesky factor
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   * Cholesky factor for a correlation matrix.
   */
  inline var_matrix_t var_cholesky_factor_corr(size_t K) {
    using stan::math::check_cholesky_factor_corr;
    var_matrix_t y(var_matrix(K, K));
    check_cholesky_factor_corr("stan::io::cholesky_factor_corr",
                               "Constrained matrix", value_of(y));
    return y;
  }

  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading from an unconstrained
   * vector of the appropriate size.
   *
   * @param K Rows and columns of Cholesky factor.
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor for a correlation matrix.
   */
  inline auto cholesky_factor_corr_constrain(size_t K) {
    return stan::math::cholesky_corr_constrain(vector((K * (K - 1)) / 2), K);
  }

  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading from an unconstrained
   * vector of the appropriate size.
   *
   * @param K Rows and columns of Cholesky factor.
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor for a correlation matrix.
   */
  inline auto var_cholesky_factor_corr_constrain(size_t K) {
    return stan::math::cholesky_corr_constrain(var_vector((K * (K - 1)) / 2), K);
  }
  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading from an unconstrained
   * vector of the appropriate size, and increment the log
   * probability reference with the log Jacobian adjustment for
   * the transform.
   *
   * @param K Rows and columns of Cholesky factor
   * @param lp Log probability reference to increment.
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor for a correlation matrix.
   */
  template <bool Jacobian = true>
  inline matrix_t cholesky_factor_corr_constrain(size_t K, T &lp) {
    if (Jacobian) {
      return stan::math::cholesky_corr_constrain(vector((K * (K - 1)) / 2), K,
                                                 lp);
    } else {
      return stan::math::cholesky_corr_constrain(vector((K * (K - 1)) / 2), K,
                                                 lp);
    }
  }

  /**
   * Return the next Cholesky factor for a correlation matrix with
   * the specified dimensionality, reading from an unconstrained
   * vector of the appropriate size, and increment the log
   * probability reference with the log Jacobian adjustment for
   * the transform.
   *
   * @param K Rows and columns of Cholesky factor
   * @param lp Log probability reference to increment.
   * @return Next Cholesky factor for a correlation matrix.
   * @throw std::domain_error if the matrix is not a valid
   *    Cholesky factor for a correlation matrix.
   */
  template <bool Jacobian = true>
  inline var_matrix_t var_cholesky_factor_corr_constrain(size_t K, T &lp) {
    if (Jacobian) {
      return stan::math::cholesky_corr_constrain(var_vector((K * (K - 1)) / 2), K,
                                                 lp);
    } else {
      return stan::math::cholesky_corr_constrain(var_vector((K * (K - 1)) / 2), K,
                                                 lp);
    }
  }

  /**
   * Return the next covariance matrix with the specified
   * dimensionality.
   *
   * <p>See <code>stan::math::check_cov_matrix(Matrix)</code>.
   *
   * @param k Dimensionality of covariance matrix.
   * @return Next covariance matrix of the specified dimensionality.
   * @throw std::runtime_error if the matrix is not a valid
   *    covariance matrix
   */
  inline map_matrix_t cov_matrix(size_t k) {
    using stan::math::value_of;
    map_matrix_t y(matrix(k, k));
    stan::math::check_cov_matrix("stan::io::cov_matrix", "Constrained matrix",
                                 value_of(y));
    return y;
  }

  /**
   * Return the next covariance matrix of the specified dimensionality.
   *
   * <p>See <code>stan::math::cov_matrix_constrain(Matrix)</code>.
   *
   * @param k Dimensionality of covariance matrix.
   * @return Next covariance matrix of the specified dimensionality.
   */
  inline auto cov_matrix_constrain(size_t k) {
    return stan::math::cov_matrix_constrain(vector(k + (k * (k - 1)) / 2), k);
  }

  /**
   * Return the next covariance matrix of the specified dimensionality,
   * incrementing the specified reference with the log absolute Jacobian
   * determinant.
   *
   * <p>See <code>stan::math::cov_matrix_constrain(Matrix,T&)</code>.
   *
   * @param k Dimensionality of the (square) covariance matrix.
   * @param lp Log probability reference to increment.
   * @return The next covariance matrix of the specified dimensionality.
   */
  template <bool Jacobian = true>
  inline matrix_t cov_matrix_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::cov_matrix_constrain(vector(k + (k * (k - 1)) / 2), k,
                                              lp);
    } else {
      return stan::math::cov_matrix_constrain(vector(k + (k * (k - 1)) / 2), k);
    }
  }

  /**
   * Return the next covariance matrix of the specified dimensionality,
   * incrementing the specified reference with the log absolute Jacobian
   * determinant.
   *
   * <p>See <code>stan::math::cov_matrix_constrain(Matrix,T&)</code>.
   *
   * @param k Dimensionality of the (square) covariance matrix.
   * @param lp Log probability reference to increment.
   * @return The next covariance matrix of the specified dimensionality.
   */
  template <bool Jacobian = true>
  inline var_matrix_t var_cov_matrix_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::cov_matrix_constrain(var_vector(k + (k * (k - 1)) / 2), k,
                                              lp);
    } else {
      return stan::math::cov_matrix_constrain(var_vector(k + (k * (k - 1)) / 2), k);
    }
  }

  /**
   * Returns the next correlation matrix of the specified dimensionality.
   *
   * <p>See <code>stan::math::check_corr_matrix(Matrix)</code>.
   *
   * @param k Dimensionality of correlation matrix.
   * @return Next correlation matrix of the specified dimensionality.
   * @throw std::runtime_error if the matrix is not a correlation matrix
   */
  inline map_matrix_t corr_matrix(size_t k) {
    map_matrix_t x(matrix(k, k));
    stan::math::check_corr_matrix("stan::math::corr_matrix",
                                  "Constrained matrix", x);
    return x;
  }

  /**
   * Returns the next correlation matrix of the specified dimensionality.
   *
   * <p>See <code>stan::math::check_corr_matrix(Matrix)</code>.
   *
   * @param k Dimensionality of correlation matrix.
   * @return Next correlation matrix of the specified dimensionality.
   * @throw std::runtime_error if the matrix is not a correlation matrix
   */
  inline var_matrix_t var_corr_matrix(size_t k) {
    using stan::math::value_of;
    var_matrix_t x(var_matrix(k, k));
    stan::math::check_corr_matrix("stan::math::corr_matrix",
                                  "Constrained matrix", value_of(x));
    return x;
  }

  /**
   * Return the next correlation matrix of the specified dimensionality.
   *
   * <p>See <code>stan::math::corr_matrix_constrain(Matrix)</code>.
   *
   * @param k Dimensionality of correlation matrix.
   * @return Next correlation matrix of the specified dimensionality.
   */
  inline auto corr_matrix_constrain(size_t k) {
    return stan::math::corr_matrix_constrain(vector((k * (k - 1)) / 2), k);
  }

  /**
   * Return the next correlation matrix of the specified dimensionality.
   *
   * <p>See <code>stan::math::corr_matrix_constrain(Matrix)</code>.
   *
   * @param k Dimensionality of correlation matrix.
   * @return Next correlation matrix of the specified dimensionality.
   */
  inline auto var_corr_matrix_constrain(size_t k) {
    return stan::math::corr_matrix_constrain(var_vector((k * (k - 1)) / 2), k);
  }

  /**
   * Return the next correlation matrix of the specified dimensionality,
   * incrementing the specified reference with the log absolute Jacobian
   * determinant.
   *
   * <p>See <code>stan::math::corr_matrix_constrain(Matrix,T&)</code>.
   *
   * @param k Dimensionality of the (square) correlation matrix.
   * @param lp Log probability reference to increment.
   * @return The next correlation matrix of the specified dimensionality.
   */
  template <bool Jacobian = true>
  inline matrix_t corr_matrix_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::corr_matrix_constrain(vector((k * (k - 1)) / 2), k, lp);
    } else {
      return stan::math::corr_matrix_constrain(vector((k * (k - 1)) / 2), k);
    }
  }

  /**
   * Return the next correlation matrix of the specified dimensionality,
   * incrementing the specified reference with the log absolute Jacobian
   * determinant.
   *
   * <p>See <code>stan::math::corr_matrix_constrain(Matrix,T&)</code>.
   *
   * @param k Dimensionality of the (square) correlation matrix.
   * @param lp Log probability reference to increment.
   * @return The next correlation matrix of the specified dimensionality.
   */
  template <bool Jacobian = true>
  inline var_matrix_t var_corr_matrix_constrain(size_t k, T &lp) {
    if (Jacobian) {
      return stan::math::corr_matrix_constrain(var_vector((k * (k - 1)) / 2), k, lp);
    } else {
      return stan::math::corr_matrix_constrain(var_vector((k * (k - 1)) / 2), k);
    }
  }

  template <typename TL>
  inline map_vector_t vector_lb(const TL lb, size_t m) {
    map_vector_t v(vector(m));
    stan::math::check_greater_or_equal("stan::io::vector_lb",
                                       "Constrained vector", v, lb);
    return v;
  }

  template <typename TL>
  inline var_vector_t vector_lb(const TL lb, size_t m) {
    using stan::math::value_of;
    var_vector_t v(var_vector(m));
    stan::math::check_greater_or_equal("stan::io::vector_lb",
                                       "Constrained vector", value_of(v), lb);
    return v;
  }

  template <typename TL>
  inline auto vector_lb_constrain(const TL lb, size_t m) {
    return stan::math::lb_constrain(vector(m), lb);
  }

  template <typename TL>
  inline auto var_vector_lb_constrain(const TL lb, size_t m) {
    return stan::math::lb_constrain(var_vector(m), lb);
  }

  template <bool Jacobian = true, typename TL>
  inline vector_t vector_lb_constrain(const TL lb, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(vector(m), lb, lp);
    } else {
      return stan::math::lb_constrain(vector(m), lb);
    }
  }

  template <bool Jacobian = true, typename TL>
  inline var_vector_t var_vector_lb_constrain(const TL lb, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(var_vector(m), lb, lp);
    } else {
      return stan::math::lb_constrain(var_vector(m), lb);
    }
  }

  template <typename TL>
  inline map_row_vector_t row_vector_lb(const TL lb, size_t m) {
    map_row_vector_t v(row_vector(m));
    stan::math::check_greater_or_equal("stan::io::row_vector_lb",
                                       "Constrained row vector", v, lb);
    return v;
  }

  template <typename TL>
  inline var_row_vector_t var_row_vector_lb(const TL lb, size_t m) {
    using stan::math::value_of;
    var_row_vector_t v(var_row_vector(m));
    stan::math::check_greater_or_equal("stan::io::row_vector_lb",
                                       "Constrained row vector", value_of(v), lb);
    return v;
  }

  template <typename TL>
  inline auto row_vector_lb_constrain(const TL lb, size_t m) {
    return stan::math::lb_constrain(row_vector(m), lb);
  }

  template <typename TL>
  inline auto var_row_vector_lb_constrain(const TL lb, size_t m) {
    return stan::math::lb_constrain(var_row_vector(m), lb);
  }

  template <bool Jacobian = true, typename TL>
  inline row_vector_t row_vector_lb_constrain(const TL lb, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(row_vector(m), lb, lp);
    } else {
      return stan::math::lb_constrain(row_vector(m), lb);
    }
  }

  template <bool Jacobian = true, typename TL>
  inline var_row_vector_t var_row_vector_lb_constrain(const TL lb, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(var_row_vector(m), lb, lp);
    } else {
      return stan::math::lb_constrain(var_row_vector(m), lb);
    }
  }

  template <typename TL>
  inline map_matrix_t matrix_lb(const TL lb, const size_t m, size_t n) {
    map_matrix_t mat(matrix(m, n));
    stan::math::check_greater_or_equal("stan::io::matrix_lb",
                                       "Constrained matrix", mat, lb);
    return mat;
  }

  template <typename TL>
  inline var_matrix_t var_matrix_lb(const TL lb, const size_t m, size_t n) {
    using stan::math::value_of;
    var_matrix_t mat(var_matrix(m, n));
    stan::math::check_greater_or_equal("stan::io::matrix_lb",
                                       "Constrained matrix", value_of(mat), lb);
    return mat;
  }

  template <typename TL>
  inline auto matrix_lb_constrain(const TL lb, size_t m, size_t n) {
    return stan::math::lb_constrain(matrix(m, n), lb);
  }

  template <typename TL>
  inline auto var_matrix_lb_constrain(const TL lb, size_t m, size_t n) {
    return stan::math::lb_constrain(var_matrix(m, n), lb);
  }

  template <bool Jacobian = true, typename TL>
  inline matrix_t matrix_lb_constrain(const TL lb, size_t m, size_t n, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(matrix(m, n), lb, lp).eval();
    } else {
      return stan::math::lb_constrain(matrix(m, n), lb).eval();
    }
  }

  template <bool Jacobian = true, typename TL>
  inline var_matrix_t var_matrix_lb_constrain(const TL lb, size_t m, size_t n, T &lp) {
    if (Jacobian) {
      return stan::math::lb_constrain(var_matrix(m, n), lb, lp).eval();
    } else {
      return stan::math::lb_constrain(var_matrix(m, n), lb).eval();
    }
  }

  template <typename TU>
  inline map_vector_t vector_ub(const TU ub, size_t m) {
    map_vector_t v(vector(m));
    stan::math::check_less_or_equal("stan::io::vector_ub", "Constrained vector",
                                    v, ub);
    return v;
  }

  template <typename TU>
  inline var_vector_t var_vector_ub(const TU ub, size_t m) {
    using stan::math::value_of;
    var_vector_t v(vector(m));
    stan::math::check_less_or_equal("stan::io::vector_ub", "Constrained vector",
                                    value_of(v), ub);
    return v;
  }

  template <typename TU>
  inline auto vector_ub_constrain(const TU ub, size_t m) {
    return stan::math::ub_constrain(vector(m), ub);
  }

  template <typename TU>
  inline auto var_vector_ub_constrain(const TU ub, size_t m) {
    return stan::math::ub_constrain(var_vector(m), ub);
  }

  template <bool Jacobian = true, typename TU>
  inline vector_t vector_ub_constrain(const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(vector(m), ub, lp);
    } else {
      return stan::math::ub_constrain(vector(m), ub);
    }
  }

  template <bool Jacobian = true, typename TU>
  inline var_vector_t var_vector_ub_constrain(const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(var_vector(m), ub, lp);
    } else {
      return stan::math::ub_constrain(var_vector(m), ub);
    }
  }

  template <typename TU>
  inline map_row_vector_t row_vector_ub(const TU ub, size_t m) {
    map_row_vector_t v(row_vector(m));
    stan::math::check_less_or_equal("stan::io::row_vector_ub",
                                    "Constrained row vector", v, ub);
    return v;
  }

  template <typename TU>
  inline var_row_vector_t var_row_vector_ub(const TU ub, size_t m) {
    using stan::math::value_of;
    var_row_vector_t v(var_row_vector(m));
    stan::math::check_less_or_equal("stan::io::row_vector_ub",
                                    "Constrained row vector", value_of(v), ub);
    return v;
  }

  template <typename TU>
  inline auto row_vector_ub_constrain(const TU ub, size_t m) {
    return stan::math::ub_constrain(row_vector(m), ub);
  }

  template <typename TU>
  inline auto var_row_vector_ub_constrain(const TU ub, size_t m) {
    return stan::math::ub_constrain(var_row_vector(m), ub);
  }

  template <bool Jacobian = true, typename TU>
  inline row_vector_t row_vector_ub_constrain(const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(row_vector(m), ub, lp);
    } else {
      return stan::math::ub_constrain(row_vector(m), ub);
    }
  }

  template <bool Jacobian = true, typename TU>
  inline var_row_vector_t var_row_vector_ub_constrain(const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(var_row_vector(m), ub, lp);
    } else {
      return stan::math::ub_constrain(var_row_vector(m), ub);
    }
  }

  template <typename TU>
  inline map_matrix_t matrix_ub(const TU ub, size_t m, size_t n) {
    map_matrix_t mat(matrix(m, n));
    stan::math::check_less_or_equal("stan::io::matrix_ub", "Constrained matrix",
                                    mat, ub);
    return mat;
  }

  template <typename TU>
  inline var_matrix_t var_matrix_ub(const TU ub, size_t m, size_t n) {
    using stan::math::value_of;
    var_matrix_t mat(var_matrix(m, n));
    stan::math::check_less_or_equal("stan::io::matrix_ub", "Constrained matrix",
                                    value_of(mat), ub);
    return mat;
  }

  template <typename TU>
  inline auto matrix_ub_constrain(const TU ub, const size_t m, size_t n) {
    return stan::math::ub_constrain(matrix(m, n), ub);
  }

  template <typename TU>
  inline auto var_matrix_ub_constrain(const TU ub, const size_t m, size_t n) {
    return stan::math::ub_constrain(var_matrix(m, n), ub);
  }

  template <bool Jacobian = true, typename TU>
  inline matrix_t matrix_ub_constrain(const TU ub, const size_t m, size_t n,
                                  T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(matrix(m, n), ub, lp);
    } else {
      return stan::math::ub_constrain(matrix(m, n), ub);
    }
  }

  template <bool Jacobian = true, typename TU>
  inline var_matrix_t var_matrix_ub_constrain(const TU ub, const size_t m, size_t n,
                                  T &lp) {
    if (Jacobian) {
      return stan::math::ub_constrain(var_matrix(m, n), ub, lp);
    } else {
      return stan::math::ub_constrain(var_matrix(m, n), ub);
    }
  }

  template <typename TL, typename TU>
  inline map_vector_t vector_lub(const TL lb, const TU ub, size_t m) {
    map_vector_t v(vector(m));
    stan::math::check_bounded<map_vector_t, TL, TU>(
        "stan::io::vector_lub", "Constrained vector", v, lb, ub);
    return v;
  }

  template <typename TL, typename TU>
  inline var_vector_t var_vector_lub(const TL lb, const TU ub, size_t m) {
    using stan::math::value_of;
    var_vector_t v(var_vector(m));
    stan::math::check_bounded<map_vector_t, TL, TU>(
        "stan::io::vector_lub", "Constrained vector", value_of(v), lb, ub);
    return v;
  }

  template <typename TL, typename TU>
  inline auto vector_lub_constrain(const TL lb, const TU ub, size_t m) {
    return stan::math::lub_constrain(vector(m), lb, ub);
  }

  template <typename TL, typename TU>
  inline auto var_vector_lub_constrain(const TL lb, const TU ub, size_t m) {
    return stan::math::lub_constrain(var_vector(m), lb, ub);
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline vector_t vector_lub_constrain(const TL lb, const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lub_constrain(vector(m), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(vector(m), lb, ub);
    }
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline var_vector_t var_vector_lub_constrain(const TL lb, const TU ub, size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lub_constrain(var_vector(m), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(var_vector(m), lb, ub);
    }
  }

  template <typename TL, typename TU>
  inline map_row_vector_t row_vector_lub(const TL lb, const TU ub, size_t m) {
    map_row_vector_t v(row_vector(m));
    stan::math::check_bounded<map_row_vector_t, TL, TU>(
        "stan::io::row_vector_lub", "Constrained row vector", v, lb, ub);
    return v;
  }

  template <typename TL, typename TU>
  inline var_row_vector_t var_row_vector_lub(const TL lb, const TU ub, size_t m) {
    using stan::math::value_of;
    var_row_vector_t v(var_row_vector(m));
    stan::math::check_bounded<map_row_vector_t, TL, TU>(
        "stan::io::row_vector_lub", "Constrained row vector", value_of(v), lb, ub);
    return v;
  }

  template <typename TL, typename TU>
  inline row_vector_t row_vector_lub_constrain(const TL lb, const TU ub,
                                               size_t m) {
    return stan::math::lub_constrain(row_vector(m), lb, ub);
  }

  template <typename TL, typename TU>
  inline var_row_vector_t var_row_vector_lub_constrain(const TL lb, const TU ub,
                                               size_t m) {
    return stan::math::lub_constrain(var_row_vector(m), lb, ub);
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline row_vector_t row_vector_lub_constrain(const TL lb, const TU ub,
                                               size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lub_constrain(row_vector(m), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(row_vector(m), lb, ub);
    }
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline var_row_vector_t var_row_vector_lub_constrain(const TL lb, const TU ub,
                                               size_t m, T &lp) {
    if (Jacobian) {
      return stan::math::lub_constrain(var_row_vector(m), lb, ub, lp);
    } else {
      return stan::math::lub_constrain(var_row_vector(m), lb, ub);
    }
  }

  template <typename TL, typename TU>
  inline map_matrix_t matrix_lub(const TL lb, const TU ub, size_t m, size_t n) {
    using stan::math::value_of;
    map_matrix_t mat(matrix(m, n));
    stan::math::check_bounded<map_matrix_t, TL, TU>(
        "stan::io::row_vector_lub", "Constrained row vector", value_of(mat), lb, ub);
    return mat;
  }

  template <typename TL, typename TU>
  inline var_matrix_t var_matrix_lub(const TL lb, const TU ub, size_t m, size_t n) {
    using stan::math::value_of;
    var_matrix_t mat(var_matrix(m, n));
    stan::math::check_bounded<map_matrix_t, TL, TU>(
        "stan::io::row_vector_lub", "Constrained row vector", value_of(mat), lb, ub);
    return mat;
  }

  template <typename TL, typename TU>
  inline auto matrix_lub_constrain(const TL lb, const TU ub, size_t m,
                                   size_t n) {
    return stan::math::lub_constrain(matrix(m, n), lb, ub);
  }

  template <typename TL, typename TU>
  inline auto var_matrix_lub_constrain(const TL lb, const TU ub, size_t m,
                                   size_t n) {
    return stan::math::lub_constrain(var_matrix(m, n), lb, ub);
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline auto matrix_lub_constrain(const TL lb, const TU ub, size_t m, size_t n,
                                   T &lp) {
    return stan::math::lub_constrain(matrix(m, n), lb, ub, lp);
  }

  template <bool Jacobian = true, typename TL, typename TU>
  inline auto var_matrix_lub_constrain(const TL lb, const TU ub, size_t m, size_t n,
                                   T &lp) {
    return stan::math::lub_constrain(var_matrix(m, n), lb, ub, lp);
  }

  template <typename TL, typename TS>
  inline map_vector_t vector_offset_multiplier(const TL offset,
                                               const TS multiplier, size_t m) {
    return vector(m);
  }

  template <typename TL, typename TS>
  inline auto var_vector_offset_multiplier(const TL offset,
                                               const TS multiplier, size_t m) {
    return var_vector(m);
  }

  template <typename TL, typename TS>
  inline auto vector_offset_multiplier_constrain(const TL offset,
                                                 const TS multiplier,
                                                 size_t m) {
    return stan::math::offset_multiplier_constrain(vector(m), offset,
                                                   multiplier);
  }

  template <typename TL, typename TS>
  inline auto var_vector_offset_multiplier_constrain(const TL offset,
                                                 const TS multiplier,
                                                 size_t m) {
    return stan::math::offset_multiplier_constrain(var_vector(m), offset,
                                                   multiplier);
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline vector_t vector_offset_multiplier_constrain(const TL offset,
                                                 const TS multiplier, size_t m,
                                                 T &lp) {
    if (Jacobian) {
      return stan::math::offset_multiplier_constrain(vector(m), offset, multiplier, lp);
    } else {
      return stan::math::offset_multiplier_constrain(vector(m), offset, multiplier);
    }
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline var_vector_t var_vector_offset_multiplier_constrain(const TL offset,
                                                 const TS multiplier, size_t m,
                                                 T &lp) {
    if (Jacobian) {
      return stan::math::offset_multiplier_constrain(var_vector(m), offset, multiplier, lp);
    } else {
      return stan::math::offset_multiplier_constrain(var_vector(m), offset, multiplier);
    }
  }

  template <typename TL, typename TS>
  inline map_row_vector_t row_vector_offset_multiplier(const TL offset,
                                                       const TS multiplier,
                                                       size_t m) {
    return row_vector(m);
  }

  template <typename TL, typename TS>
  inline var_row_vector_t var_row_vector_offset_multiplier(const TL offset,
                                                       const TS multiplier,
                                                       size_t m) {
    return var_row_vector(m);
  }

  template <typename TL, typename TS>
  inline auto row_vector_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m) {
    return stan::math::offset_multiplier_constrain(row_vector(m), offset, multiplier);
  }

  template <typename TL, typename TS>
  inline auto var_row_vector_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m) {
    return stan::math::offset_multiplier_constrain(var_row_vector(m), offset, multiplier);
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline row_vector_t row_vector_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, T &lp) {
    return stan::math::offset_multiplier_constrain(row_vector(m), offset, multiplier, lp);
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline var_row_vector_t var_row_vector_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, T &lp) {
    return stan::math::offset_multiplier_constrain(var_row_vector(m), offset, multiplier, lp);
  }

  template <typename TL, typename TS>
  inline map_matrix_t matrix_offset_multiplier(const TL offset, const TS multiplier, size_t m, size_t n) {
    return matrix(m, n);
  }

  template <typename TL, typename TS>
  inline var_matrix_t var_matrix_offset_multiplier(const TL offset, const TS multiplier, size_t m, size_t n) {
    return var_matrix(m, n);
  }

  template <typename TL, typename TS>
  inline auto matrix_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, size_t n) {
    return stan::math::offset_multiplier_constrain(matrix(m, n), offset, multiplier);
  }

  template <typename TL, typename TS>
  inline auto var_matrix_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, size_t n) {
    return stan::math::offset_multiplier_constrain(var_matrix(m, n), offset, multiplier);
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline matrix_t matrix_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, size_t n, T &lp) {
    if (Jacobian) {
      return stan::math::offset_multiplier_constrain(matrix(m, n), offset, multiplier, lp);
    } else {
      return stan::math::offset_multiplier_constrain(matrix(m, n), offset, multiplier);
    }
  }

  template <bool Jacobian = true, typename TL, typename TS>
  inline var_matrix_t var_matrix_offset_multiplier_constrain(const TL offset, const TS multiplier, size_t m, size_t n, T &lp) {
    if (Jacobian) {
      return stan::math::offset_multiplier_constrain(var_matrix(m, n), offset, multiplier, lp);
    } else {
      return stan::math::offset_multiplier_constrain(var_matrix(m, n), offset, multiplier);
    }
  }
};

}  // namespace io

}  // namespace stan

#endif
