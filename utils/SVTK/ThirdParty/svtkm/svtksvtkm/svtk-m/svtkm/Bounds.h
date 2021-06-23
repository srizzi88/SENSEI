//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Bounds_h
#define svtk_m_Bounds_h

#include <svtkm/Range.h>

namespace svtkm
{

/// \brief Represent an axis-aligned 3D bounds in space.
///
/// \c svtkm::Bounds is a helper class for representing the axis-aligned box
/// representing some region in space. The typical use of this class is to
/// express the containing box of some geometry. The box is specified as ranges
/// in the x, y, and z directions.
///
/// \c Bounds also contains several helper functions for computing and
/// maintaining the bounds.
///
struct Bounds
{
  svtkm::Range X;
  svtkm::Range Y;
  svtkm::Range Z;

  SVTKM_EXEC_CONT
  Bounds() {}

  Bounds(const Bounds&) = default;

  SVTKM_EXEC_CONT
  Bounds(const svtkm::Range& xRange, const svtkm::Range& yRange, const svtkm::Range& zRange)
    : X(xRange)
    , Y(yRange)
    , Z(zRange)
  {
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
  SVTKM_EXEC_CONT Bounds(const T1& minX,
                        const T2& maxX,
                        const T3& minY,
                        const T4& maxY,
                        const T5& minZ,
                        const T6& maxZ)
    : X(svtkm::Range(minX, maxX))
    , Y(svtkm::Range(minY, maxY))
    , Z(svtkm::Range(minZ, maxZ))
  {
  }

  /// Initialize bounds with an array of 6 values in the order xmin, xmax,
  /// ymin, ymax, zmin, zmax.
  ///
  template <typename T>
  SVTKM_EXEC_CONT explicit Bounds(const T bounds[6])
    : X(svtkm::Range(bounds[0], bounds[1]))
    , Y(svtkm::Range(bounds[2], bounds[3]))
    , Z(svtkm::Range(bounds[4], bounds[5]))
  {
  }

  /// Initialize bounds with the minimum corner point and the maximum corner
  /// point.
  ///
  template <typename T>
  SVTKM_EXEC_CONT Bounds(const svtkm::Vec<T, 3>& minPoint, const svtkm::Vec<T, 3>& maxPoint)
    : X(svtkm::Range(minPoint[0], maxPoint[0]))
    , Y(svtkm::Range(minPoint[1], maxPoint[1]))
    , Z(svtkm::Range(minPoint[2], maxPoint[2]))
  {
  }

  svtkm::Bounds& operator=(const svtkm::Bounds& src) = default;

  /// \b Determine if the bounds are valid (i.e. has at least one valid point).
  ///
  /// \c IsNonEmpty returns true if the bounds contain some valid points. If
  /// the bounds are any real region, even if a single point or it expands to
  /// infinity, true is returned.
  ///
  SVTKM_EXEC_CONT
  bool IsNonEmpty() const
  {
    return (this->X.IsNonEmpty() && this->Y.IsNonEmpty() && this->Z.IsNonEmpty());
  }

  /// \b Determines if a point coordinate is within the bounds.
  ///
  template <typename T>
  SVTKM_EXEC_CONT bool Contains(const svtkm::Vec<T, 3>& point) const
  {
    return (this->X.Contains(point[0]) && this->Y.Contains(point[1]) && this->Z.Contains(point[2]));
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the point at the middle of the bounds. If the bounds
  /// are empty, the results are undefined.
  ///
  SVTKM_EXEC_CONT
  svtkm::Vec3f_64 Center() const
  {
    return svtkm::Vec3f_64(this->X.Center(), this->Y.Center(), this->Z.Center());
  }

  /// \b Expand bounds to include a point.
  ///
  /// This version of \c Include expands the bounds just enough to include the
  /// given point coordinates. If the bounds already include this point, then
  /// nothing is done.
  ///
  template <typename T>
  SVTKM_EXEC_CONT void Include(const svtkm::Vec<T, 3>& point)
  {
    this->X.Include(point[0]);
    this->Y.Include(point[1]);
    this->Z.Include(point[2]);
  }

  /// \b Expand bounds to include other bounds.
  ///
  /// This version of \c Include expands these bounds just enough to include
  /// that of another bounds. Essentially it is the union of the two bounds.
  ///
  SVTKM_EXEC_CONT
  void Include(const svtkm::Bounds& bounds)
  {
    this->X.Include(bounds.X);
    this->Y.Include(bounds.Y);
    this->Z.Include(bounds.Z);
  }

  /// \b Return the union of this and another bounds.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  SVTKM_EXEC_CONT
  svtkm::Bounds Union(const svtkm::Bounds& otherBounds) const
  {
    svtkm::Bounds unionBounds(*this);
    unionBounds.Include(otherBounds);
    return unionBounds;
  }

  /// \b Operator for union
  ///
  SVTKM_EXEC_CONT
  svtkm::Bounds operator+(const svtkm::Bounds& otherBounds) const { return this->Union(otherBounds); }

  SVTKM_EXEC_CONT
  bool operator==(const svtkm::Bounds& bounds) const
  {
    return ((this->X == bounds.X) && (this->Y == bounds.Y) && (this->Z == bounds.Z));
  }

  SVTKM_EXEC_CONT
  bool operator!=(const svtkm::Bounds& bounds) const
  {
    return ((this->X != bounds.X) || (this->Y != bounds.Y) || (this->Z != bounds.Z));
  }
};

/// Helper function for printing bounds during testing
///
static inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::Bounds& bounds)
{
  return stream << "{ X:" << bounds.X << ", Y:" << bounds.Y << ", Z:" << bounds.Z << " }";
}

} // namespace svtkm

#endif //svtk_m_Bounds_h
