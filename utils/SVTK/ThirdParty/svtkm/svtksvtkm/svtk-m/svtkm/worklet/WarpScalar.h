//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_WarpScalar_h
#define svtk_m_worklet_WarpScalar_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace worklet
{
// A functor that modify points by moving points along point normals by the scalar
// amount times the scalar factor. It's a SVTK-m version of the svtkWarpScalar in SVTK.
// Useful for creating carpet or x-y-z plots.
// It doesn't modify the point coordinates, but creates a new point coordinates
// that have been warped.
class WarpScalar
{
public:
  class WarpScalarImp : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, _4);
    SVTKM_CONT
    WarpScalarImp(svtkm::FloatDefault scaleAmount)
      : ScaleAmount(scaleAmount)
    {
    }

    SVTKM_EXEC
    void operator()(const svtkm::Vec3f& point,
                    const svtkm::Vec3f& normal,
                    const svtkm::FloatDefault& scaleFactor,
                    svtkm::Vec3f& result) const
    {
      result = point + this->ScaleAmount * scaleFactor * normal;
    }

    template <typename T1, typename T2, typename T3>
    SVTKM_EXEC void operator()(const svtkm::Vec<T1, 3>& point,
                              const svtkm::Vec<T2, 3>& normal,
                              const T3& scaleFactor,
                              svtkm::Vec<T1, 3>& result) const
    {
      result = point + static_cast<T1>(this->ScaleAmount * scaleFactor) * normal;
    }


  private:
    svtkm::FloatDefault ScaleAmount;
  };

  // Execute the WarpScalar worklet given the points, vector and a scale factor.
  // Scale factor can differs per point.
  template <typename PointType,
            typename NormalType,
            typename ScaleFactorType,
            typename ResultType,
            typename ScaleAmountType>
  void Run(PointType point,
           NormalType normal,
           ScaleFactorType scaleFactor,
           ScaleAmountType scale,
           ResultType warpedPoint)
  {
    WarpScalarImp warpScalarImp(scale);
    svtkm::worklet::DispatcherMapField<WarpScalarImp> dispatcher(warpScalarImp);
    dispatcher.Invoke(point, normal, scaleFactor, warpedPoint);
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_WarpScalar_h
