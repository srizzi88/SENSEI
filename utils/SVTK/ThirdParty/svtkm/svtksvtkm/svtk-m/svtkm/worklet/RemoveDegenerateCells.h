//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_RemoveDegeneratePolygons_h
#define svtk_m_worklet_RemoveDegeneratePolygons_h

#include <svtkm/worklet/DispatcherMapTopology.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetPermutation.h>

#include <svtkm/worklet/CellDeepCopy.h>

#include <svtkm/CellTraits.h>

#include <svtkm/exec/CellFace.h>

namespace svtkm
{
namespace worklet
{

struct RemoveDegenerateCells
{
  struct IdentifyDegenerates : svtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell);
    using ExecutionSignature = _2(CellShape, PointIndices);
    using InputDomain = _1;

    template <svtkm::IdComponent dimensionality, typename CellShapeTag, typename PointVecType>
    SVTKM_EXEC bool CheckForDimensionality(svtkm::CellTopologicalDimensionsTag<dimensionality>,
                                          CellShapeTag,
                                          PointVecType&& pointIds) const
    {
      const svtkm::IdComponent numPoints = pointIds.GetNumberOfComponents();
      svtkm::IdComponent numUnduplicatedPoints = 0;
      for (svtkm::IdComponent localPointId = 0; localPointId < numPoints; ++localPointId)
      {
        ++numUnduplicatedPoints;
        if (numUnduplicatedPoints >= dimensionality + 1)
        {
          return true;
        }
        while (((localPointId < numPoints - 1) &&
                (pointIds[localPointId] == pointIds[localPointId + 1])) ||
               ((localPointId == numPoints - 1) && (pointIds[localPointId] == pointIds[0])))
        {
          // Skip over any repeated points. Assume any repeated points are adjacent.
          ++localPointId;
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointVecType>
    SVTKM_EXEC bool CheckForDimensionality(svtkm::CellTopologicalDimensionsTag<0>,
                                          CellShapeTag,
                                          PointVecType&&)
    {
      return true;
    }

    template <typename CellShapeTag, typename PointVecType>
    SVTKM_EXEC bool CheckForDimensionality(svtkm::CellTopologicalDimensionsTag<3>,
                                          CellShapeTag shape,
                                          PointVecType&& pointIds)
    {
      const svtkm::IdComponent numFaces = svtkm::exec::CellFaceNumberOfFaces(shape, *this);
      svtkm::Id numValidFaces = 0;
      for (svtkm::IdComponent faceId = 0; faceId < numFaces; ++faceId)
      {
        if (this->CheckForDimensionality(
              svtkm::CellTopologicalDimensionsTag<2>(), svtkm::CellShapeTagPolygon(), pointIds))
        {
          ++numValidFaces;
          if (numValidFaces > 2)
          {
            return true;
          }
        }
      }
      return false;
    }

    template <typename CellShapeTag, typename PointIdVec>
    SVTKM_EXEC bool operator()(CellShapeTag shape, const PointIdVec& pointIds) const
    {
      using Traits = svtkm::CellTraits<CellShapeTag>;
      return this->CheckForDimensionality(
        typename Traits::TopologicalDimensionsTag(), shape, pointIds);
    }

    template <typename PointIdVec>
    SVTKM_EXEC bool operator()(svtkm::CellShapeTagGeneric shape, PointIdVec&& pointIds) const
    {
      bool passCell = true;
      switch (shape.Id)
      {
        svtkmGenericCellShapeMacro(passCell = (*this)(CellShapeTag(), pointIds));
        default:
          // Raise an error for unknown cell type? Pass if we don't know.
          passCell = true;
      }
      return passCell;
    }
  };

  template <typename CellSetType>
  svtkm::cont::CellSetExplicit<> Run(const CellSetType& cellSet)
  {
    svtkm::cont::ArrayHandle<bool> passFlags;
    DispatcherMapTopology<IdentifyDegenerates> dispatcher;
    dispatcher.Invoke(cellSet, passFlags);

    svtkm::cont::ArrayHandleCounting<svtkm::Id> indices =
      svtkm::cont::make_ArrayHandleCounting(svtkm::Id(0), svtkm::Id(1), passFlags.GetNumberOfValues());
    svtkm::cont::Algorithm::CopyIf(
      svtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()), passFlags, this->ValidCellIds);

    svtkm::cont::CellSetPermutation<CellSetType> permutation(this->ValidCellIds, cellSet);
    svtkm::cont::CellSetExplicit<> output;
    svtkm::worklet::CellDeepCopy::Run(permutation, output);
    return output;
  }

  struct CallWorklet
  {
    template <typename CellSetType>
    void operator()(const CellSetType& cellSet,
                    RemoveDegenerateCells& self,
                    svtkm::cont::CellSetExplicit<>& output) const
    {
      output = self.Run(cellSet);
    }
  };

  template <typename CellSetList>
  svtkm::cont::CellSetExplicit<> Run(const svtkm::cont::DynamicCellSetBase<CellSetList>& cellSet)
  {
    svtkm::cont::CellSetExplicit<> output;
    cellSet.CastAndCall(CallWorklet(), *this, output);

    return output;
  }

  template <typename ValueType, typename StorageTag>
  svtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const svtkm::cont::ArrayHandle<ValueType, StorageTag> in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = svtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    svtkm::cont::ArrayHandle<ValueType> result;
    svtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  svtkm::cont::ArrayHandle<svtkm::Id> ValidCellIds;
};
}
}

#endif //svtk_m_worklet_RemoveDegeneratePolygons_h
