//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_TetrahedralizeExplicit_h
#define svtk_m_worklet_TetrahedralizeExplicit_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapter.h>
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

/// \brief Compute the tetrahedralize cells for an explicit grid data set
class TetrahedralizeExplicit
{
public:
  TetrahedralizeExplicit() {}

  //
  // Worklet to count the number of tetrahedra generated per cell
  //
  class TetrahedraPerCell : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn shapes, ExecObject tables, FieldOut tetrahedronCount);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    SVTKM_CONT
    TetrahedraPerCell() {}

    template <typename DeviceAdapter>
    SVTKM_EXEC svtkm::IdComponent operator()(
      svtkm::UInt8 shape,
      const svtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter>& tables)
      const
    {
      return tables.GetCount(svtkm::CellShapeTagGeneric(shape));
    }
  };

  //
  // Worklet to turn cells into tetrahedra
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TetrahedralizeCell : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  ExecObject tables,
                                  FieldOutCell connectivityOut);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3, VisitIndex);
    using InputDomain = _1;

    using ScatterType = svtkm::worklet::ScatterCounting;

    template <typename CellArrayType>
    SVTKM_CONT static ScatterType MakeScatter(const CellArrayType& cellArray)
    {
      return ScatterType(cellArray);
    }

    // Each cell produces tetrahedra and write result at the offset
    template <typename CellShapeTag,
              typename ConnectivityInVec,
              typename DeviceAdapter,
              typename ConnectivityOutVec>
    SVTKM_EXEC void operator()(
      CellShapeTag shape,
      const ConnectivityInVec& connectivityIn,
      const svtkm::worklet::internal::TetrahedralizeTablesExecutionObject<DeviceAdapter>& tables,
      ConnectivityOutVec& connectivityOut,
      svtkm::IdComponent visitIndex) const
    {
      svtkm::IdComponent4 tetIndices = tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[tetIndices[0]];
      connectivityOut[1] = connectivityIn[tetIndices[1]];
      connectivityOut[2] = connectivityIn[tetIndices[2]];
      connectivityOut[3] = connectivityIn[tetIndices[3]];
    }
  };

  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(
    const CellSetType& svtkmNotUsed(cellSet),
    svtkm::cont::ArrayHandle<svtkm::IdComponent>& svtkmNotUsed(outCellsPerCell))
  {
    return svtkm::cont::CellSetSingleType<>();
  }

  svtkm::cont::CellSetSingleType<> Run(const svtkm::cont::CellSetExplicit<>& cellSet,
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

    svtkm::worklet::internal::TetrahedralizeTables tables;

    // Determine the number of output cells each input cell will generate
    svtkm::worklet::DispatcherMapField<TetrahedraPerCell> tetPerCellDispatcher;
    tetPerCellDispatcher.Invoke(inShapes, tables.PrepareForInput(), outCellsPerCell);

    // Build new cells
    svtkm::worklet::DispatcherMapTopology<TetrahedralizeCell> tetrahedralizeDispatcher(
      TetrahedralizeCell::MakeScatter(outCellsPerCell));
    tetrahedralizeDispatcher.Invoke(
      cellSet, tables.PrepareForInput(), svtkm::cont::make_ArrayHandleGroupVec<4>(outConnectivity));

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), svtkm::CellShapeTagTetra::Id, 4, outConnectivity);
    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_TetrahedralizeExplicit_h
