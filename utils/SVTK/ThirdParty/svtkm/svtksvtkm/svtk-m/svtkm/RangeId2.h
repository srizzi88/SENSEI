//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_RangeId2_h
#define svtk_m_RangeId2_h

#include <svtkm/RangeId.h>

namespace svtkm
{

/// \brief Represent 2D integer range.
///
/// \c svtkm::RangeId2 is a helper class for representing a 2D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x, y, and z directions.
///
/// \c RangeId2 also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId2
{
  svtkm::RangeId X;
  svtkm::RangeId Y;

  RangeId2() = default;

  SVTKM_EXEC_CONT
  RangeId2(const svtkm::RangeId& xrange, const svtkm::RangeId& yrange)
    : X(xrange)
    , Y(yrange)
  {
  }

  SVTKM_EXEC_CONT
  RangeId2(svtkm::Id minX, svtkm::Id maxX, svtkm::Id minY, svtkm::Id maxY)
    : X(svtkm::RangeId(minX, maxX))
    , Y(svtkm::RangeId(minY, maxY))
  {
  }

  /// Initialize range with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  SVTKM_EXEC_CONT
  explicit RangeId2(const svtkm::Id range[4])
    : X(svtkm::RangeId(range[0], range[1]))
    , Y(svtkm::RangeId(range[2], range[3]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  SVTKM_EXEC_CONT
  RangeId2(const svtkm::Id2& min, const svtkm::Id2& max)
    : X(svtkm::RangeId(min[0], max[0]))
    , Y(svtkm::RangeId(min[1], max[1]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  SVTKM_EXEC_CONT
  bool IsNonEmpty() const { return (this->X.IsNonEmpty() && this->Y.IsNonEmpty()); }

  /// \b Determines if an Id2 value is within the range.
  ///
  SVTKM_EXEC_CONT
  bool Contains(const svtkm::Id2& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  SVTKM_EXEC_CONT
  svtkm::Id2 Center() const { return svtkm::Id2(this->X.Center(), this->Y.Center()); }

  SVTKM_EXEC_CONT
  svtkm::Id2 Dimensions() const { return svtkm::Id2(this->X.Length(), this->Y.Length()); }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  SVTKM_EXEC_CONT void Include(const svtkm::Vec<T, 2>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  SVTKM_EXEC_CONT
  void Include(const svtkm::RangeId2& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId2 Union(const svtkm::RangeId2& other) const
  {
    svtkm::RangeId2 unionRangeId2(*this);
    unionRangeId2.Include(other);
    return unionRangeId2;
  }

  /// \b Operator for union
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId2 operator+(const svtkm::RangeId2& other) const { return this->Union(other); }

  SVTKM_EXEC_CONT
  bool operator==(const svtkm::RangeId2& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y));
  }

  SVTKM_EXEC_CONT
  bool operator!=(const svtkm::RangeId2& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y));
  }

  SVTKM_EXEC_CONT
  svtkm::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }

  SVTKM_EXEC_CONT
  const svtkm::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else
    {
      return this->Y;
    }
  }
};

} // namespace svtkm

/// Helper function for printing range during testing
///
static inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::RangeId2& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << " }";
}

#endif //svtk_m_RangeId2_h
