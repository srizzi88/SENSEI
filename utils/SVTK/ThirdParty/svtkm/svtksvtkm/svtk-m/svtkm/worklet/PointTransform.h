//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_PointTransform_h
#define svtk_m_worklet_PointTransform_h

#include <svtkm/Math.h>
#include <svtkm/Matrix.h>
#include <svtkm/Transform3D.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{

template <typename T>
class PointTransform : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  SVTKM_CONT
  PointTransform() {}

  //Translation
  SVTKM_CONT void SetTranslation(const T& tx, const T& ty, const T& tz)
  {
    matrix = svtkm::Transform3DTranslate(tx, ty, tz);
  }

  SVTKM_CONT void SetTranslation(const svtkm::Vec<T, 3>& v) { SetTranslation(v[0], v[1], v[2]); }

  //Rotation
  SVTKM_CONT void SetRotation(const T& angleDegrees, const svtkm::Vec<T, 3>& axis)
  {
    matrix = svtkm::Transform3DRotate(angleDegrees, axis);
  }

  SVTKM_CONT void SetRotation(const T& angleDegrees, const T& rx, const T& ry, const T& rz)
  {
    SetRotation(angleDegrees, { rx, ry, rz });
  }

  SVTKM_CONT void SetRotationX(const T& angleDegrees) { SetRotation(angleDegrees, 1, 0, 0); }

  SVTKM_CONT void SetRotationY(const T& angleDegrees) { SetRotation(angleDegrees, 0, 1, 0); }

  SVTKM_CONT void SetRotationZ(const T& angleDegrees) { SetRotation(angleDegrees, 0, 0, 1); }

  //Scaling
  SVTKM_CONT void SetScale(const T& s) { matrix = svtkm::Transform3DScale(s, s, s); }

  SVTKM_CONT void SetScale(const T& sx, const T& sy, const T& sz)
  {
    matrix = svtkm::Transform3DScale(sx, sy, sz);
  }

  SVTKM_CONT void SetScale(const svtkm::Vec<T, 3>& v)
  {
    matrix = svtkm::Transform3DScale(v[0], v[1], v[2]);
  }

  //General transformation
  SVTKM_CONT
  void SetTransform(const svtkm::Matrix<T, 4, 4>& mtx) { matrix = mtx; }

  //Functor
  SVTKM_EXEC
  svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& vec) const
  {
    return svtkm::Transform3DPoint(matrix, vec);
  }

private:
  svtkm::Matrix<T, 4, 4> matrix;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_PointTransform_h
