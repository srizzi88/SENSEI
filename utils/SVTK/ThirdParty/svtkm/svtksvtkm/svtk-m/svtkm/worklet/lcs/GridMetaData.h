//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_lcs_GridMetaData_h
#define svtk_m_worklet_lcs_GridMetaData_h

namespace svtkm
{
namespace worklet
{
namespace detail
{

class GridMetaData
{
public:
  using Structured2DType = svtkm::cont::CellSetStructured<2>;
  using Structured3DType = svtkm::cont::CellSetStructured<3>;

  SVTKM_CONT
  GridMetaData(const svtkm::cont::DynamicCellSet cellSet)
  {
    if (cellSet.IsType<Structured2DType>())
    {
      this->cellSet2D = true;
      svtkm::Id2 dims =
        cellSet.Cast<Structured2DType>().GetSchedulingRange(svtkm::TopologyElementTagPoint());
      this->Dims = svtkm::Id3(dims[0], dims[1], 1);
    }
    else
    {
      this->cellSet2D = false;
      this->Dims =
        cellSet.Cast<Structured3DType>().GetSchedulingRange(svtkm::TopologyElementTagPoint());
    }
    this->PlaneSize = Dims[0] * Dims[1];
    this->RowSize = Dims[0];
  }

  SVTKM_EXEC
  bool IsCellSet2D() const { return this->cellSet2D; }

  SVTKM_EXEC
  void GetLogicalIndex(const svtkm::Id index, svtkm::Id3& logicalIndex) const
  {
    logicalIndex[0] = index % Dims[0];
    logicalIndex[1] = (index / Dims[0]) % Dims[1];
    if (this->cellSet2D)
      logicalIndex[2] = 0;
    else
      logicalIndex[2] = index / (Dims[0] * Dims[1]);
  }

  SVTKM_EXEC
  const svtkm::Vec<svtkm::Id, 6> GetNeighborIndices(const svtkm::Id index) const
  {
    svtkm::Vec<svtkm::Id, 6> indices;
    svtkm::Id3 logicalIndex;
    GetLogicalIndex(index, logicalIndex);

    // For differentials w.r.t delta in x
    indices[0] = (logicalIndex[0] == 0) ? index : index - 1;
    indices[1] = (logicalIndex[0] == Dims[0] - 1) ? index : index + 1;
    // For differentials w.r.t delta in y
    indices[2] = (logicalIndex[1] == 0) ? index : index - RowSize;
    indices[3] = (logicalIndex[1] == Dims[1] - 1) ? index : index + RowSize;
    if (!this->cellSet2D)
    {
      // For differentials w.r.t delta in z
      indices[4] = (logicalIndex[2] == 0) ? index : index - PlaneSize;
      indices[5] = (logicalIndex[2] == Dims[2] - 1) ? index : index + PlaneSize;
    }
    return indices;
  }

private:
  bool cellSet2D = false;
  svtkm::Id3 Dims;
  svtkm::Id PlaneSize;
  svtkm::Id RowSize;
};

} // namespace detail
} // namespace worklet
} // namespace svtkm

#endif //svtk_m_worklet_lcs_GridMetaData_h
