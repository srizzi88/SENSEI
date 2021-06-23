//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_PointElevation_h
#define svtk_m_worklet_PointElevation_h

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/Math.h>

namespace svtkm
{
namespace worklet
{

namespace internal
{

template <typename T>
SVTKM_EXEC T clamp(const T& val, const T& min, const T& max)
{
  return svtkm::Min(max, svtkm::Max(min, val));
}
}

class PointElevation : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  SVTKM_CONT
  PointElevation()
    : LowPoint(0.0, 0.0, 0.0)
    , HighPoint(0.0, 0.0, 1.0)
    , RangeLow(0.0)
    , RangeHigh(1.0)
  {
  }

  SVTKM_CONT
  void SetLowPoint(const svtkm::Vec3f_64& point) { this->LowPoint = point; }

  SVTKM_CONT
  void SetHighPoint(const svtkm::Vec3f_64& point) { this->HighPoint = point; }

  SVTKM_CONT
  void SetRange(svtkm::Float64 low, svtkm::Float64 high)
  {
    this->RangeLow = low;
    this->RangeHigh = high;
  }

  SVTKM_EXEC
  svtkm::Float64 operator()(const svtkm::Vec3f_64& vec) const
  {
    svtkm::Vec3f_64 direction = this->HighPoint - this->LowPoint;
    svtkm::Float64 lengthSqr = svtkm::Dot(direction, direction);
    svtkm::Float64 rangeLength = this->RangeHigh - this->RangeLow;
    svtkm::Float64 s = svtkm::Dot(vec - this->LowPoint, direction) / lengthSqr;
    s = internal::clamp(s, 0.0, 1.0);
    return this->RangeLow + (s * rangeLength);
  }

  template <typename T>
  SVTKM_EXEC svtkm::Float64 operator()(const svtkm::Vec<T, 3>& vec) const
  {
    return (*this)(svtkm::make_Vec(static_cast<svtkm::Float64>(vec[0]),
                                  static_cast<svtkm::Float64>(vec[1]),
                                  static_cast<svtkm::Float64>(vec[2])));
  }

private:
  svtkm::Vec3f_64 LowPoint, HighPoint;
  svtkm::Float64 RangeLow, RangeHigh;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_PointElevation_h
