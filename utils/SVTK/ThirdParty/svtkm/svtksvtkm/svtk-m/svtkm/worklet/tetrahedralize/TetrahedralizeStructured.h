//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_TetrahedralizeStructured_h
#define svtk_m_worklet_TetrahedralizeStructured_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterUniform.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{

namespace tetrahedralize
{
//
// Worklet to turn hexahedra into tetrahedra
// Vertices remain the same and each cell is processed with needing topology
//
class TetrahedralizeCell : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, ThreadIndices);
  using InputDomain = _1;

  using ScatterType = svtkm::worklet::ScatterUniform<5>;

  // Each hexahedron cell produces five tetrahedron cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec, typename ThreadIndicesType>
  SVTKM_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                            ConnectivityOutVec& connectivityOut,
                            const ThreadIndicesType threadIndices) const
  {
    SVTKM_STATIC_CONSTEXPR_ARRAY svtkm::IdComponent StructuredTetrahedronIndices[2][5][4] = {
      { { 0, 1, 3, 4 }, { 1, 4, 5, 6 }, { 1, 4, 6, 3 }, { 1, 3, 6, 2 }, { 3, 6, 7, 4 } },
      { { 2, 1, 5, 0 }, { 0, 2, 3, 7 }, { 2, 5, 6, 7 }, { 0, 7, 4, 5 }, { 0, 2, 7, 5 } }
    };

    svtkm::Id3 inputIndex = threadIndices.GetInputIndex3D();

    // Calculate the type of tetrahedron generated because it alternates
    svtkm::Id indexType = (inputIndex[0] + inputIndex[1] + inputIndex[2]) % 2;

    svtkm::IdComponent visitIndex = threadIndices.GetVisitIndex();

    connectivityOut[0] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][2]];
    connectivityOut[3] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][3]];
  }
};
}

/// \brief Compute the tetrahedralize cells for a uniform grid data set
class TetrahedralizeStructured
{
public:
  template <typename CellSetType>
  svtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      svtkm::cont::ArrayHandle<svtkm::IdComponent>& outCellsPerCell)
  {
    svtkm::cont::CellSetSingleType<> outCellSet;
    svtkm::cont::ArrayHandle<svtkm::Id> connectivity;

    svtkm::worklet::DispatcherMapTopology<tetrahedralize::TetrahedralizeCell> dispatcher;
    dispatcher.Invoke(cellSet, svtkm::cont::make_ArrayHandleGroupVec<4>(connectivity));

    // Fill in array of output cells per input cell
    svtkm::cont::ArrayCopy(
      svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(5, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), svtkm::CellShapeTagTetra::Id, 4, connectivity);
    return outCellSet;
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_TetrahedralizeStructured_h
