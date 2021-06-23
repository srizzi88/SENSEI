//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_WarpVector_h
#define svtk_m_worklet_WarpVector_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace worklet
{
// A functor that modify points by moving points along a vector
// then timing a scale factor. It's a SVTK-m version of the svtkWarpVector in SVTK.
// Useful for showing flow profiles or mechanical deformation.
// This worklet does not modify the input points but generate new point coordinate
// instance that has been warped.
class WarpVector
{
public:
  class WarpVectorImp : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, FieldIn, FieldOut);
    using ExecutionSignature = _3(_1, _2);
    SVTKM_CONT
    WarpVectorImp(svtkm::FloatDefault scale)
      : Scale(scale)
    {
    }

    SVTKM_EXEC
    svtkm::Vec3f operator()(const svtkm::Vec3f& point, const svtkm::Vec3f& vector) const
    {
      return point + this->Scale * vector;
    }

    template <typename T>
    SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& point,
                                         const svtkm::Vec<T, 3>& vector) const
    {
      return point + static_cast<T>(this->Scale) * vector;
    }


  private:
    svtkm::FloatDefault Scale;
  };

  // Execute the WarpVector worklet given the points, vector and a scale factor.
  // Returns:
  // warped points
  template <typename PointType, typename VectorType, typename ResultType>
  void Run(PointType point, VectorType vector, svtkm::FloatDefault scale, ResultType warpedPoint)
  {
    WarpVectorImp warpVectorImp(scale);
    svtkm::worklet::DispatcherMapField<WarpVectorImp> dispatcher(warpVectorImp);
    dispatcher.Invoke(point, vector, warpedPoint);
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_WarpVector_h
