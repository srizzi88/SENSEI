//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PointTransform_hxx
#define svtk_m_filter_PointTransform_hxx
#include <svtkm/filter/PointTransform.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT PointTransform::PointTransform()
  : Worklet()
  , ChangeCoordinateSystem(true)
{
  this->SetOutputFieldName("transform");
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetTranslation(const svtkm::FloatDefault& tx,
                                                     const svtkm::FloatDefault& ty,
                                                     const svtkm::FloatDefault& tz)
{
  this->Worklet.SetTranslation(tx, ty, tz);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetTranslation(const svtkm::Vec3f& v)
{
  this->Worklet.SetTranslation(v);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetRotation(const svtkm::FloatDefault& angleDegrees,
                                                  const svtkm::Vec3f& axis)
{
  this->Worklet.SetRotation(angleDegrees, axis);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetRotation(const svtkm::FloatDefault& angleDegrees,
                                                  const svtkm::FloatDefault& rx,
                                                  const svtkm::FloatDefault& ry,
                                                  const svtkm::FloatDefault& rz)
{
  this->Worklet.SetRotation(angleDegrees, rx, ry, rz);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetRotationX(const svtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationX(angleDegrees);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetRotationY(const svtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationY(angleDegrees);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetRotationZ(const svtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationZ(angleDegrees);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetScale(const svtkm::FloatDefault& s)
{
  this->Worklet.SetScale(s);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetScale(const svtkm::FloatDefault& sx,
                                               const svtkm::FloatDefault& sy,
                                               const svtkm::FloatDefault& sz)
{
  this->Worklet.SetScale(sx, sy, sz);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetScale(const svtkm::Vec3f& v)
{
  this->Worklet.SetScale(v);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetTransform(
  const svtkm::Matrix<svtkm::FloatDefault, 4, 4>& mtx)
{
  this->Worklet.SetTransform(mtx);
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void PointTransform::SetChangeCoordinateSystem(bool flag)
{
  this->ChangeCoordinateSystem = flag;
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT bool PointTransform::GetChangeCoordinateSystem() const
{
  return this->ChangeCoordinateSystem;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet PointTransform::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<T> outArray;
  this->Invoke(this->Worklet, field, outArray);

  svtkm::cont::DataSet outData =
    CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);

  if (this->GetChangeCoordinateSystem())
  {
    svtkm::Id coordIndex =
      this->GetUseCoordinateSystemAsField() ? this->GetActiveCoordinateSystemIndex() : 0;
    outData.GetCoordinateSystem(coordIndex).SetData(outArray);
  }

  return outData;
}
}
} // namespace svtkm::filter

#endif //svtk_m_filter_PointTransform_hxx
