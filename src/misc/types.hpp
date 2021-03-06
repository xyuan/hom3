#ifndef HOM3_MISC_TYPES_HPP_
#define HOM3_MISC_TYPES_HPP_
////////////////////////////////////////////////////////////////////////////////
/// \file \brief Basic types used througout HOM3
////////////////////////////////////////////////////////////////////////////////
/// Includes:
#include <Eigen/Dense>
#include <boost/range.hpp>
#include <boost/range/config.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/mutable_iterator.hpp>
#include <boost/range/difference_type.hpp>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include "misc/assert.hpp"
#include "misc/returns.hpp"
////////////////////////////////////////////////////////////////////////////////

/// \brief Contains hom3's code
namespace hom3 {


/// \name Basic Types
///@{
using Ind    = int64_t;
using SInd   = int32_t;
using Num    = double;
using Int    = int64_t;
using SInt   = int32_t;
using String = std::string;
///@}

/// \name Eigen aliases
///@{
template<class T, SInd nd>
using EigenColMajor = Eigen::Matrix<T, Eigen::Dynamic, nd, Eigen::ColMajor>;
template<class T, SInd nd>
using EigenRowMajor = Eigen::Matrix<T, Eigen::Dynamic, nd, Eigen::RowMajor>;
template<class T, SInd nRows>
using EigenStaticVector = Eigen::Matrix<T, nRows, 1>;
template<class T, SInd nRows, SInd nCols>
using EigenStaticColMajor = Eigen::Matrix<T, nRows, nCols, Eigen::ColMajor>;
template<class T> using EigenDynColMajor
= Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;
template<class T> using EigenDynRowMajor
= Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

///@}

/// \name Dynamic vector types (constant but dynamic length)
///@{
using IntV = EigenColMajor<Int, SInd(1)>;
using IndV = EigenColMajor<Ind, SInd(1)>;
using NumV = EigenColMajor<Num, SInd(1)>;
///@}

/// \name Static vector types (length is compile-time constant)
///@{
template<SInd nRows> using NumA  = EigenStaticVector<Num, nRows>;
template<SInd nRows> using IntA  = EigenStaticVector<Int, nRows>;
template<SInd nRows> using IndA  = EigenStaticVector<Ind, nRows>;
template<SInd nRows> using SIndA = EigenStaticVector<SInd, nRows>;
template<SInd nRows> using SIntA = EigenStaticVector<SInt, nRows>;

using EBV = Eigen::Matrix<bool, Eigen::Dynamic, 1>;
///@}

/// \name Col-major matrix types (dynamic length, static # of columns)
///@{
template<SInd nd = 1> using IntM  = EigenColMajor< Int, nd>;
template<SInd nd = 1> using SIntM = EigenColMajor<SInt, nd>;
template<SInd nd = 1> using IndM  = EigenColMajor< Ind, nd>;
template<SInd nd = 1> using SIndM = EigenColMajor<SInd, nd>;
template<SInd nd = 1> using NumM  = EigenColMajor< Num, nd>;
///@}

/// \name Row-major matrix types (dynamic length, static # of columns)
///@{
template<SInd nd = 1> using IndRM = EigenRowMajor<Ind, nd>;
///@}

/// \name Col-major static matrix type (nRows, nCols both compile-time
/// constants)
///
///@{
template<SInd nRows, SInd nCols>
using NumAM = EigenStaticColMajor<Num, nRows, nCols>;
///@}

}  // namespace hom3

////////////////////////////////////////////////////////////////////////////////
namespace Eigen {
////////////////////////////////////////////////////////////////////////////////

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto begin(Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto end(Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data() + v.size());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto begin(const Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto end(const Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data() + v.size());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto range_begin(Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto range_end(Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data() + v.size());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto range_begin
(const Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data());

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
auto range_end
(const Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& v)
RETURNS(v.data() + v.size());

////////////////////////////////////////////////////////////////////////////////
}  // namespace Eigen
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
namespace boost {
////////////////////////////////////////////////////////////////////////////////

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
inline BOOST_DEDUCED_TYPENAME range_difference<
  Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>>::type
range_calculate_size
(const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>& rng)
{ return rng.size(); }

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
struct range_mutable_iterator<
  Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>
> { using type = _Scalar*; };

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
struct range_const_iterator<
  Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>
> { using type = const _Scalar*; };

template<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
         int _MaxCols, int BlockRows, int BlockCols, bool InnerPanel>
struct range_const_iterator
<Eigen::Block<
  Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>,
  BlockRows, BlockCols, InnerPanel
  >
> { using type = const _Scalar*; };

template<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows,
         int _MaxCols, int BlockRows, int BlockCols, bool InnerPanel>
struct range_const_iterator
<Eigen::Block<
  const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>,
  BlockRows, BlockCols, InnerPanel
  >
> { using type = const _Scalar*; };

template
<class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols>
struct range_iterator<
  Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>
> { using type = _Scalar*; };

////////////////////////////////////////////////////////////////////////////////
}  // namespace boost
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#endif
