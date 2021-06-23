//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_TriangulateExplicit_h
#define svtk_m_worklet_TriangulateExplicit_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/worklet/internal/TriangulateTables.h>

namespace svtkm
{
namespace worklet
{

/// \brief Compute the triangulate cells for an explicit grid data set
class TriangulateExplicit
{
public:
  TriangulateExplicit() {}

  //
  // Worklet to count the number of triangles generated per cell
  //
  class TrianglesPerCell : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn shapes,
                                  FieldIn numPoints,
                                  ExecObject tables,
                                  FieldOut triangleCount);
    using ExecutionSignature = _4(_1, _2, _3);
    using InputDomain = _1;

    SVTKM_CONT
    TrianglesPerCell() {}

    template <typename DeviceAdapter>
    SVTKM_EXEC svtkm::IdComponent operator()(
      svtkm::UInt8 shape,
      svtkm::IdComponent numPoints,
      const svtkm::worklet::internal::TriangulateTablesExecutionObject<DeviceAdapter>& tables) const
    {
      return tables.GetCount(svtkm::CellShapeTagGeneric(shape), numPoints);
    }
  };

  //
  // Worklet to turn cells into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  ExecObject tables,
                                  FieldOutCell connectivityOut);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3, VisitIndex);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    SVTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    // Each cell produces triangles and write result at the offset
    template <typename CellShapeTag,
              typename ConnectivityInVec,
              typename ConnectivityOutVec,
              typename DeviceAdapter>
    SVTKM_EXEC void operator()(
      CellShapeTag shape,
      const ConnectivityInVec& connectivityIn,
      const svtkm::worklet::internal::TriangulateTablesExecutionObject<DeviceAdapter>& tables,
      ConnectivityOutVec& connectivityOut,
      svtkm::IdComponent visitIndex) const
    {
      svtkm::IdComponent3 triIndices = tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[triIndices[0]];
      connectivityOut[1] = connectivityIn[triIndices[1]];
      connectivityOut[2] = connectivityIn[triIndices[2]];
    }
  };
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(
    const CellSetType& svtkmNotUsed(cellSet),
    svtkm::cont::ArrayHandle<svtkm::IdComponent>& svtkmNotUsed(outCellsPerCell))
  {
    svtkm::cont::CellSetSingleType<> outCellSet;
    return outCellSet;
  }
};
template <>
svtkm::cont::CellSetSingleType<> TriangulateExplicit::Run(
  const svtkm::cont::CellSetExplicit<>& cellSet,
  svtkm::cont::ArrayHandle<svtkm::IdComponent>& outCellsPerCell)
{
  svtkm::cont::CellSetSingleType<> outCellSet;

  // Input topology
  auto inShapes =
    cellSet.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
  auto inNumIndices =
    cellSet.GetNumIndicesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

  // Output topology
  svtkm::cont::ArrayHandle<svtkm::Id> outConnectivity;

  svtkm::worklet::internal::TriangulateTables tables;

  // Determine the number of output cells each input cell will generate
  svtkm::worklet::DispatcherMapField<TrianglesPerCell> triPerCellDispatcher;
  triPerCellDispatcher.Invoke(inShapes, inNumIndices, tables.PrepareForInput(), outCellsPerCell);

  // Build new cells
  svtkm::worklet::DispatcherMapTopology<TriangulateCell> triangulateDispatcher(
    TriangulateCell::MakeScatter(outCellsPerCell));
  triangulateDispatcher.Invoke(
    cellSet, tables.PrepareForInput(), svtkm::cont::make_ArrayHandleGroupVec<3>(outConnectivity));

  // Add cells to output cellset
  outCellSet.Fill(cellSet.GetNumberOfPoints(), svtkm::CellShapeTagTriangle::Id, 3, outConnectivity);
  return outCellSet;
}
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_TriangulateExplicit_h
