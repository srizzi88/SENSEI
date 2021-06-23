//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_RangeId3_h
#define svtk_m_RangeId3_h

#include <svtkm/RangeId.h>

namespace svtkm
{

/// \brief Represent 3D integer range.
///
/// \c svtkm::RangeId3 is a helper class for representing a 3D range of integer
/// values. The typical use of this class is to express a box of indices
/// in the x, y, and z directions.
///
/// \c RangeId3 also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId3
{
  svtkm::RangeId X;
  svtkm::RangeId Y;
  svtkm::RangeId Z;

  RangeId3() = default;

  SVTKM_EXEC_CONT
  RangeId3(const svtkm::RangeId& xrange, const svtkm::RangeId& yrange, const svtkm::RangeId& zrange)
    : X(xrange)
    , Y(yrange)
    , Z(zrange)
  {
  }

  SVTKM_EXEC_CONT
  RangeId3(svtkm::Id minX, svtkm::Id maxX, svtkm::Id minY, svtkm::Id maxY, svtkm::Id minZ, svtkm::Id maxZ)
    : X(svtkm::RangeId(minX, maxX))
    , Y(svtkm::RangeId(minY, maxY))
    , Z(svtkm::RangeId(minZ, maxZ))
  {
  }

  /// Initialize range with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  SVTKM_EXEC_CONT
  explicit RangeId3(const svtkm::Id range[6])
    : X(svtkm::RangeId(range[0], range[1]))
    , Y(svtkm::RangeId(range[2], range[3]))
    , Z(svtkm::RangeId(range[4], range[5]))
  {
  }

  /// Initialize range with the minimum and the maximum corners
  ///
  SVTKM_EXEC_CONT
  RangeId3(const svtkm::Id3& min, const svtkm::Id3& max)
    : X(svtkm::RangeId(min[0], max[0]))
    , Y(svtkm::RangeId(min[1], max[1]))
    , Z(svtkm::RangeId(min[2], max[2]))
  {
  }

  /// \b Determine if the range is non-empty.
  ///
  /// \c IsNonEmpty returns true if the range is non-empty.
  ///
  SVTKM_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if an Id3 value is within the range.
  ///
  SVTKM_EXEC_CONT
  bool Contains(const svtkm::Id3& val) const
  {
    return (this->X.Contains(val[0]) && this->Y.Contains(val[1]) && this->Z.Contains(val[2]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle of the range.
  ///
  SVTKM_EXEC_CONT
  svtkm::Id3 Center() const
  {
    return svtkm::Id3(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  SVTKM_EXEC_CONT
  svtkm::Id3 Dimensions() const
  {
    return svtkm::Id3(this->X.Length(), this->Y.Length(), this->Z.Length());
  }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already include this value, then
  /// nothing is done.
  ///
  template <typename T>
  SVTKM_EXEC_CONT void Include(const svtkm::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands the range just enough to include
  /// the other range. Essentially it is the union of the two ranges.
  ///
  SVTKM_EXEC_CONT
  void Include(const svtkm::RangeId3& range)
  {
    this->X.Include(range.X);
    this->Y.Include(range.Y);
    this->Z.Include(range.Z);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId3 Union(const svtkm::RangeId3& other) const
  {
    svtkm::RangeId3 unionRangeId3(*this);
    unionRangeId3.Include(other);
    return unionRangeId3;
  }

  /// \b Operator for union
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId3 operator+(const svtkm::RangeId3& other) const { return this->Union(other); }

  SVTKM_EXEC_CONT
  bool operator==(const svtkm::RangeId3& range) const
  {
    return ((this->X == range.X) && (this->Y == range.Y) && (this->Z == range.Z));
  }

  SVTKM_EXEC_CONT
  bool operator!=(const svtkm::RangeId3& range) const
  {
    return ((this->X != range.X) || (this->Y != range.Y) || (this->Z != range.Z));
  }
  SVTKM_EXEC_CONT
  svtkm::RangeId& operator[](IdComponent c) noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }

  SVTKM_EXEC_CONT
  const svtkm::RangeId& operator[](IdComponent c) const noexcept
  {
    if (c <= 0)
    {
      return this->X;
    }
    else if (c == 1)
    {
      return this->Y;
    }
    else
    {
      return this->Z;
    }
  }
};

/// Helper function for printing range during testing
///
inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::RangeId3& range)
{
  return stream << "{ X:" << range.X << ", Y:" << range.Y << ", Z:" << range.Z << " }";
} // Declared inside of svtkm namespace so that the operator work with ADL lookup
} // namespace svtkm

#endif //svtk_m_RangeId3_h
