//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_TwoLevelUniformGridExecutonObject_h
#define svtk_m_exec_TwoLevelUniformGridExecutonObject_h

#include <svtkm/cont/ArrayHandle.h>



namespace svtkm
{
namespace exec
{
namespace twolevelgrid
{
using DimensionType = svtkm::Int16;
using DimVec3 = svtkm::Vec<DimensionType, 3>;
using FloatVec3 = svtkm::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};
template <typename Device>
struct TwoLevelUniformGridExecutionObject
{


  template <typename T>
  using ArrayPortalConst =
    typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<Device>::PortalConst;

  Grid TopLevel;

  ArrayPortalConst<DimVec3> LeafDimensions;
  ArrayPortalConst<svtkm::Id> LeafStartIndex;

  ArrayPortalConst<svtkm::Id> CellStartIndex;
  ArrayPortalConst<svtkm::Id> CellCount;
  ArrayPortalConst<svtkm::Id> CellIds;
};
}
}
}
#endif // svtk_m_cont_TwoLevelUniformGridExecutonObject_h
