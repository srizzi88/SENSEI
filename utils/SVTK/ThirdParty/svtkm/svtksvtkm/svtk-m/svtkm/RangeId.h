//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_RangeId_h
#define svtk_m_RangeId_h

#include <svtkm/Math.h>
#include <svtkm/Types.h>

namespace svtkm
{

/// \brief Represent a range of svtkm::Id values.
///
/// \c svtkm::RangeId is a helper class for representing a range of svtkm::Id
/// values. This is specified simply with a \c Min and \c Max value, where
/// \c Max is exclusive.
///
/// \c RangeId also contains several helper functions for computing and
/// maintaining the range.
///
struct RangeId
{
  svtkm::Id Min;
  svtkm::Id Max;

  SVTKM_EXEC_CONT
  RangeId()
    : Min(0)
    , Max(0)
  {
  }

  SVTKM_EXEC_CONT
  RangeId(svtkm::Id min, svtkm::Id max)
    : Min(min)
    , Max(max)
  {
  }

  /// \b Determine if the range is valid.
  ///
  /// \c IsNonEmpty return true if the range contains some valid values between
  /// \c Min and \c Max. If \c Max <= \c Min, then no values satisfy
  /// the range and \c IsNonEmpty returns false. Otherwise, return true.
  ///
  SVTKM_EXEC_CONT
  bool IsNonEmpty() const { return (this->Min < this->Max); }

  /// \b Determines if a value is within the range.
  ///
  /// \c Contains returns true if the give value is within the range, false
  /// otherwise.
  ///
  SVTKM_EXEC_CONT
  bool Contains(svtkm::Id value) const { return ((this->Min <= value) && (this->Max > value)); }

  /// \b Returns the length of the range.
  ///
  /// \c Length computes the distance between the min and max. If the range
  /// is empty, 0 is returned.
  ///
  SVTKM_EXEC_CONT
  svtkm::Id Length() const { return this->Max - this->Min; }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle value of the range.
  ///
  SVTKM_EXEC_CONT
  svtkm::Id Center() const { return (this->Min + this->Max) / 2; }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already includes this value, then nothing is
  /// done.
  ///
  SVTKM_EXEC_CONT
  void Include(svtkm::Id value)
  {
    this->Min = svtkm::Min(this->Min, value);
    this->Max = svtkm::Max(this->Max, value + 1);
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands this range just enough to include that
  /// of another range. Essentially it is the union of the two ranges.
  ///
  SVTKM_EXEC_CONT
  void Include(const svtkm::RangeId& range)
  {
    this->Min = svtkm::Min(this->Min, range.Min);
    this->Max = svtkm::Max(this->Max, range.Max);
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId Union(const svtkm::RangeId& other) const
  {
    svtkm::RangeId unionRange(*this);
    unionRange.Include(other);
    return unionRange;
  }

  /// \b Operator for union
  ///
  SVTKM_EXEC_CONT
  svtkm::RangeId operator+(const svtkm::RangeId& other) const { return this->Union(other); }

  SVTKM_EXEC_CONT
  bool operator==(const svtkm::RangeId& other) const
  {
    return ((this->Min == other.Min) && (this->Max == other.Max));
  }

  SVTKM_EXEC_CONT
  bool operator!=(const svtkm::RangeId& other) const
  {
    return ((this->Min != other.Min) || (this->Max != other.Max));
  }
};

/// Helper function for printing ranges during testing
///
static inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::RangeId& range)
{
  return stream << "[" << range.Min << ".." << range.Max << ")";
} // Declared inside of svtkm namespace so that the operator work with ADL lookup
} // namespace svtkm

#endif // svtk_m_RangeId_h
