#ifndef HOM3_INTERPOLATION_RBF_HPP_
#define HOM3_INTERPOLATION_RBF_HPP_
////////////////////////////////////////////////////////////////////////////////
#include <Eigen/SVD>
#include "misc/types.hpp"
////////////////////////////////////////////////////////////////////////////////
/// \file \brief Implements interpolation with Radial Basis Functions
////////////////////////////////////////////////////////////////////////////////
namespace hom3 { namespace interpolation {
////////////////////////////////////////////////////////////////////////////////

/// \brief Interpolation with Radial Basis Functions
namespace rbf {

/// \brief Radial Basis Functions
namespace kernel {

/// \brief Gaussian
struct Gaussian {
  inline Num operator()(const Num d) const noexcept
  { return std::exp(-std::pow(d * eps, 2.)); }
  const Num eps{1.0};  ///< Width
};

/// \brief Multiquadric
struct Multiquadric {
  inline Num operator()(const Num d) const noexcept
  { return std::sqrt(1. + std::pow(d * eps, 2.)); }
  const Num eps{1.0};  ///< Width
};

/// \brief Inverse quadratic
struct InverseQuadratic {
  inline Num operator()(const Num d) const noexcept
  { return 1./ (1. + std::pow(d * eps, 2.)); }
  const Num eps{1.0};  ///< Width
};

/// \brief Inverse Multiquadric
struct InverseMultiquadric {
  InverseMultiquadric(const Num e) : eps(e) {}
  inline Num operator()(const Num d) const noexcept
  { return 1. / std::sqrt(1. + std::pow(d * eps, 2.)); }
  const Num eps{1.0};  ///< Width
};

/// \brief Thin plate
struct ThinPlate {
  inline Num operator()(const Num d) const noexcept
  { return math::approx(d, 0.) ? 0. : std::pow(d, 2.) * std::log(d); }
};

}  // namespace kernel

namespace detail {

template<class Vector, class Kernel>
inline Eigen::JacobiSVD<Eigen::MatrixXd> build_system
(const std::vector<Vector>& x_samples, const Kernel kernel) {
  const auto length = x_samples.size();
  Eigen::MatrixXd m(length, length);
  for (std::size_t i = 0; i < length; ++i)  {
    for (std::size_t j = 0; j < length; ++j) {
      m(i, j) = kernel((x_samples[i] - x_samples[j]).norm());
    }
  }
  return {m, Eigen::ComputeThinU | Eigen::ComputeThinV};
}

inline Eigen::MatrixXd build_weights
(Eigen::JacobiSVD<Eigen::MatrixXd> system, const std::vector<Num>& values) {
  const Eigen::Map<const Eigen::VectorXd> valuesWrapper(values.data(),
                                                        values.size());
  return system.solve(valuesWrapper);
}

}  // namespace detail

/// \brief Returns weights for a single variable
template<class Vector, class Kernel = kernel::Gaussian>
Eigen::MatrixXd build_weights
(const std::vector<Vector>& x_samples, const std::vector<Num>& values,
 const Kernel kernel = Kernel()) {
  ASSERT(x_samples.size() > 0, "Zero samples!");
  ASSERT(values.size() > 0, "Zero values!");
  return detail::build_weights(detail::build_system(x_samples, kernel), values);
}

/// \brief Returns weights for multiple variables
template<class Vector, class Kernel = kernel::Gaussian>
std::vector<Eigen::MatrixXd> build_weights
(const std::vector<Vector>& x_samples,
 const std::vector<std::vector<Num>>& vector_values,
 const Kernel kernel = Kernel()) {
  ASSERT(vector_values.size() > 0, "Zero vector values!");
  std::vector<Eigen::MatrixXd> result;
  result.reserve(vector_values.size());
  for (auto&& values : vector_values) {
    result.push_back(build_weights(x_samples, values, kernel));
  }
  return result;
}

/// \brief Interpolates a single variable at \p point
template<class Vector, class Kernel = kernel::Gaussian>
Num interpolate(const Vector& point, const std::vector<Vector>& x_samples,
                const Eigen::MatrixXd& weights,
                const Kernel kernel = Kernel()) {
  ASSERT(x_samples.size() > 0, "Zero samples!");
  const auto length = x_samples.size();
  Num result = 0.0;
  for (std::size_t i = 0; i < length; ++i) {
    result += weights(i) * kernel((point - x_samples[i]).norm());
  }
  return result;
}

/// \brief Interpolates multiple variables at \p point
template<class Vector, class Kernel = kernel::Gaussian>
std::vector<Num> interpolate(const Vector& point,
                             const std::vector<Vector>& x_samples,
                             const std::vector<Eigen::MatrixXd>& weights,
                             const Kernel kernel = Kernel()) {
  ASSERT(x_samples.size() > 0, "Zero samples!");
  ASSERT(weights.size() > 0, "Zero weights!");
  const auto length = x_samples.size();
  const auto nvars = weights.size();
  std::vector<Num> results(nvars, 0.0);
  for (std::size_t i = 0; i < length; ++i) {
    const auto k = kernel((point - x_samples[i]).norm());
    for (std::size_t w = 0; w < nvars; ++w) {
      results[w] += weights[w](i) * k;
    }
  }
  return results;
}

}  // namespace rbf

////////////////////////////////////////////////////////////////////////////////
}  // namespace interpolation
}  // namespace hom3
////////////////////////////////////////////////////////////////////////////////
#endif