#ifndef HOM3_SOLVERS_FV_ADVECTION_TAGS_HPP_
#define HOM3_SOLVERS_FV_ADVECTION_TAGS_HPP_
////////////////////////////////////////////////////////////////////////////////
/// \file \brief Defines the Advection equation's tags.
////////////////////////////////////////////////////////////////////////////////
namespace hom3 { namespace solver { namespace fv { namespace advection {
////////////////////////////////////////////////////////////////////////////////

/// \brief Advection equation physic's tag
struct type_tag {};

/// \brief Advection equation numerical-flux tags
namespace flux {
struct local_lax_friedrichs {};
struct upwind {};
}  // namespace flux

////////////////////////////////////////////////////////////////////////////////
}  // namespace advection
}  // namespace fv
}  // namespace solver
}  // namespace hom3
////////////////////////////////////////////////////////////////////////////////
#endif