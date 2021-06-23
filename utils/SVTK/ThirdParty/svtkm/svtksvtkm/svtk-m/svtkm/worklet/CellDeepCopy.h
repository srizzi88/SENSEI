//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_CellDeepCopy_h
#define svtk_m_worklet_CellDeepCopy_h

#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DynamicCellSet.h>

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{

/// Container for worklets and helper methods to copy a cell set to a new
/// \c CellSetExplicit structure
///
struct CellDeepCopy
{
  struct CountCellPoints : svtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut numPointsInCell);
    using ExecutionSignature = _2(PointCount);

    SVTKM_EXEC
    svtkm::IdComponent operator()(svtkm::IdComponent numPoints) const { return numPoints; }
  };

  struct PassCellStructure : svtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn inputTopology, FieldOut shapes, FieldOut pointIndices);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    SVTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              svtkm::UInt8& outShape,
                              OutPointIndexType& outPoints) const
    {
      (void)inShape; //C4100 false positive workaround
      outShape = inShape.Id;

      svtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      SVTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (svtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  SVTKM_CONT static void Run(
    const InCellSetType& inCellSet,
    svtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    SVTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;

    svtkm::worklet::DispatcherMapTopology<CountCellPoints> countDispatcher;
    countDispatcher.Invoke(inCellSet, numIndices);

    svtkm::cont::ArrayHandle<svtkm::UInt8, ShapeStorage> shapes;
    svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorage> connectivity;

    svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorage> offsets;
    svtkm::Id connectivitySize;
    svtkm::cont::ConvertNumIndicesToOffsets(numIndices, offsets, connectivitySize);
    connectivity.Allocate(connectivitySize);

    auto offsetsTrim =
      svtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

    svtkm::worklet::DispatcherMapTopology<PassCellStructure> passDispatcher;
    passDispatcher.Invoke(
      inCellSet, shapes, svtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsetsTrim));

    svtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage> newCellSet;
    newCellSet.Fill(inCellSet.GetNumberOfPoints(), shapes, connectivity, offsets);
    outCellSet = newCellSet;
  }

  template <typename InCellSetType>
  SVTKM_CONT static svtkm::cont::CellSetExplicit<> Run(const InCellSetType& inCellSet)
  {
    SVTKM_IS_DYNAMIC_OR_STATIC_CELL_SET(InCellSetType);

    svtkm::cont::CellSetExplicit<> outCellSet;
    Run(inCellSet, outCellSet);

    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_CellDeepCopy_h
