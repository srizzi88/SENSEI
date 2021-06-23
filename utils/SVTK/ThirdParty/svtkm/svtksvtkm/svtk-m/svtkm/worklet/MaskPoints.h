//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtkm_m_worklet_MaskPoints_h
#define svtkm_m_worklet_MaskPoints_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class MaskPoints
{
public:
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet, const svtkm::Id stride)
  {
    svtkm::Id numberOfInputPoints = cellSet.GetNumberOfPoints();
    svtkm::Id numberOfSampledPoints = numberOfInputPoints / stride;
    svtkm::cont::ArrayHandleCounting<svtkm::Id> strideArray(0, stride, numberOfSampledPoints);

    svtkm::cont::ArrayHandle<svtkm::Id> pointIds;
    svtkm::cont::ArrayCopy(strideArray, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    svtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(numberOfInputPoints, svtkm::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif // svtkm_m_worklet_MaskPoints_h
