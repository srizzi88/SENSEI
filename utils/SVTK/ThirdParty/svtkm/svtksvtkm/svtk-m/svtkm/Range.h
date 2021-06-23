//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_Range_h
#define svtk_m_Range_h

#include <svtkm/Assert.h>
#include <svtkm/Math.h>
#include <svtkm/Types.h>

namespace svtkm
{

/// \brief Represent a continuous scalar range of values.
///
/// \c svtkm::Range is a helper class for representing a range of floating point
/// values from a minimum value to a maximum value. This is specified simply
/// enough with a \c Min and \c Max value.
///
/// \c Range also contains several helper functions for computing and
/// maintaining the range.
///
struct Range
{
  svtkm::Float64 Min;
  svtkm::Float64 Max;

  SVTKM_EXEC_CONT
  Range()
    : Min(svtkm::Infinity64())
    , Max(svtkm::NegativeInfinity64())
  {
  }

  Range(const Range&) = default;
  Range(Range&&) = default;

  template <typename T1, typename T2>
  SVTKM_EXEC_CONT Range(const T1& min, const T2& max)
    : Min(static_cast<svtkm::Float64>(min))
    , Max(static_cast<svtkm::Float64>(max))
  {
  }

  svtkm::Range& operator=(const svtkm::Range& src) = default;
  svtkm::Range& operator=(svtkm::Range&& src) = default;

  /// \b Determine if the range is valid (i.e. has at least one valid point).
  ///
  /// \c IsNonEmpty return true if the range contains some valid values between
  /// \c Min and \c Max. If \c Max is less than \c Min, then no values satisfy
  /// the range and \c IsNonEmpty returns false. Otherwise, return true.
  ///
  /// \c IsNonEmpty assumes \c Min and \c Max are inclusive. That is, if they
  /// are equal then true is returned.
  ///
  SVTKM_EXEC_CONT
  bool IsNonEmpty() const { return (this->Min <= this->Max); }

  /// \b Determines if a value is within the range.
  ///
  /// \c Contains returns true if the give value is within the range, false
  /// otherwise. \c Contains treats the min and max as inclusive. That is, if
  /// the value is exactly the min or max, true is returned.
  ///
  template <typename T>
  SVTKM_EXEC_CONT bool Contains(const T& value) const
  {
    return ((this->Min <= static_cast<svtkm::Float64>(value)) &&
            (this->Max >= static_cast<svtkm::Float64>(value)));
  }

  /// \b Returns the length of the range.
  ///
  /// \c Length computes the distance between the min and max. If the range
  /// is empty, 0 is returned.
  ///
  SVTKM_EXEC_CONT
  svtkm::Float64 Length() const
  {
    if (this->IsNonEmpty())
    {
      return (this->Max - this->Min);
    }
    else
    {
      return 0.0;
    }
  }

  /// \b Returns the center of the range.
  ///
  /// \c Center computes the middle value of the range. If the range is empty,
  /// NaN is returned.
  ///
  SVTKM_EXEC_CONT
  svtkm::Float64 Center() const
  {
    if (this->IsNonEmpty())
    {
      return 0.5 * (this->Max + this->Min);
    }
    else
    {
      return svtkm::Nan64();
    }
  }

  /// \b Expand range to include a value.
  ///
  /// This version of \c Include expands the range just enough to include the
  /// given value. If the range already includes this value, then nothing is
  /// done.
  ///
  template <typename T>
  SVTKM_EXEC_CONT void Include(const T& value)
  {
    this->Min = svtkm::Min(this->Min, static_cast<svtkm::Float64>(value));
    this->Max = svtkm::Max(this->Max, static_cast<svtkm::Float64>(value));
  }

  /// \b Expand range to include other range.
  ///
  /// This version of \c Include expands this range just enough to include that
  /// of another range. Essentially it is the union of the two ranges.
  ///
  SVTKM_EXEC_CONT
  void Include(const svtkm::Range& range)
  {
    if (range.IsNonEmpty())
    {
      this->Min = svtkm::Min(this->Min, range.Min);
      this->Max = svtkm::Max(this->Max, range.Max);
    }
  }

  /// \b Return the union of this and another range.
  ///
  /// This is a nondestructive form of \c Include.
  ///
  SVTKM_EXEC_CONT
  svtkm::Range Union(const svtkm::Range& otherRange) const
  {
    svtkm::Range unionRange(*this);
    unionRange.Include(otherRange);
    return unionRange;
  }

  /// \b Operator for union
  ///
  SVTKM_EXEC_CONT
  svtkm::Range operator+(const svtkm::Range& otherRange) const { return this->Union(otherRange); }

  SVTKM_EXEC_CONT
  bool operator==(const svtkm::Range& otherRange) const
  {
    return ((this->Min == otherRange.Min) && (this->Max == otherRange.Max));
  }

  SVTKM_EXEC_CONT
  bool operator!=(const svtkm::Range& otherRange) const
  {
    return ((this->Min != otherRange.Min) || (this->Max != otherRange.Max));
  }
};

/// Helper function for printing ranges during testing
///
inline SVTKM_CONT std::ostream& operator<<(std::ostream& stream, const svtkm::Range& range)
{
  return stream << "[" << range.Min << ".." << range.Max << "]";
} // Declared inside of svtkm namespace so that the operator work with ADL lookup
} // namespace svtkm


#endif //svtk_m_Range_h
