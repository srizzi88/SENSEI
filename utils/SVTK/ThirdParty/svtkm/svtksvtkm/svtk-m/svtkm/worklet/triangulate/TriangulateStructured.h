//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_TriangulateStructured_h
#define svtk_m_worklet_TriangulateStructured_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterUniform.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{
namespace triangulate
{
//
// Worklet to turn quads into triangles
// Vertices remain the same and each cell is processed with needing topology
//
class TriangulateCell : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, VisitIndex);
  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterUniform<2>;

  // Each quad cell produces 2 triangle cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec>
  SVTKM_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                            ConnectivityOutVec& connectivityOut,
                            svtkm::IdComponent visitIndex) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::IdComponent StructuredTriangleIndices[2][3] = { { 0, 1, 2 },
                                                                                      { 0, 2, 3 } };
    connectivityOut[0] = connectivityIn[StructuredTriangleIndices[visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTriangleIndices[visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTriangleIndices[visitIndex][2]];
  }
};
}

/// \brief Compute the triangulate cells for a uniform grid data set
class TriangulateStructured
{
public:
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      svtkm::cont::ArrayHandle<svtkm::IdComponent>& outCellsPerCell)

  {
    svtkm::cont::CellSetSingleType<> outCellSet;
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;

    svtkm::worklet::DispatcherMapTopology<triangulate::TriangulateCell> dispatcher;
    dispatcher.Invoke(cellSet, svtkm::cont::make_ArrayHandleGroupVec<3>(connectivity));

    // Fill in array of output cells per input cell
    svtkm::cont::ArrayCopy(
      svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(2, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), svtkm::CellShapeTagTriangle::Id, 3, connectivity);
    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_TriangulateStructured_h
