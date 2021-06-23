//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_internal_ConnectivityStructuredInternals_h
#define svtk_m_internal_ConnectivityStructuredInternals_h

#include <svtkm/CellShape.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/TopologyElementTag.h>
#include <svtkm/Types.h>
#include <svtkm/VecVariable.h>

#include <svtkm/internal/Assume.h>

namespace svtkm
{
namespace internal
{

template <svtkm::IdComponent>
class ConnectivityStructuredInternals;

//1 D specialization.
template <>
class ConnectivityStructuredInternals<1>
{
public:
  using SchedulingRangeType = svtkm::Id;

  SVTKM_EXEC_CONT
  void SetPointDimensions(svtkm::Id dimensions) { this->PointDimensions = dimensions; }

  SVTKM_EXEC_CONT
  void SetGlobalPointIndexStart(svtkm::Id start) { this->GlobalPointIndexStart = start; }

  SVTKM_EXEC_CONT
  svtkm::Id GetPointDimensions() const { return this->PointDimensions; }

  SVTKM_EXEC_CONT
  svtkm::Id GetCellDimensions() const { return this->PointDimensions - 1; }

  SVTKM_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(svtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  SVTKM_EXEC_CONT
  SchedulingRangeType GetSchedulingRange(svtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  SVTKM_EXEC_CONT
  SchedulingRangeType GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr svtkm::IdComponent NUM_POINTS_IN_CELL = 2;
  static constexpr svtkm::IdComponent MAX_CELL_TO_POINT = 2;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfPoints() const { return this->PointDimensions; }
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfCells() const { return this->PointDimensions - 1; }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetCellShape() const { return svtkm::CELL_SHAPE_LINE; }

  using CellShapeTag = svtkm::CellShapeTagLine;

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(svtkm::Id index) const
  {
    SVTKM_ASSUME(index >= 0);

    svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = index;
    pointIds[1] = pointIds[0] + 1;
    return pointIds;
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfCellsOnPoint(svtkm::Id pointIndex) const
  {
    SVTKM_ASSUME(pointIndex >= 0);

    if ((pointIndex > 0) && (pointIndex < this->PointDimensions - 1))
    {
      return 2;
    }
    else
    {
      return 1;
    }
  }

  SVTKM_EXEC_CONT
  svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(svtkm::Id index) const
  {
    SVTKM_ASSUME(index >= 0);
    SVTKM_ASSUME(this->PointDimensions > 1);

    svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if (index > 0)
    {
      cellIds.Append(index - 1);
    }
    if (index < this->PointDimensions - 1)
    {
      cellIds.Append(index);
    }

    return cellIds;
  }


  SVTKM_EXEC_CONT
  svtkm::Id FlatToLogicalPointIndex(svtkm::Id flatPointIndex) const { return flatPointIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatPointIndex(svtkm::Id logicalPointIndex) const { return logicalPointIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id FlatToLogicalCellIndex(svtkm::Id flatCellIndex) const { return flatCellIndex; }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatCellIndex(svtkm::Id logicalCellIndex) const { return logicalCellIndex; }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<1> ";
    out << "this->PointDimensions[" << this->PointDimensions << "] ";
    out << "\n";
  }

private:
  svtkm::Id PointDimensions = 0;
  svtkm::Id GlobalPointIndexStart = 0;
};

//2 D specialization.
template <>
class ConnectivityStructuredInternals<2>
{
public:
  using SchedulingRangeType = svtkm::Id2;

  SVTKM_EXEC_CONT
  void SetPointDimensions(svtkm::Id2 dims) { this->PointDimensions = dims; }

  SVTKM_EXEC_CONT
  void SetGlobalPointIndexStart(svtkm::Id2 start) { this->GlobalPointIndexStart = start; }

  SVTKM_EXEC_CONT
  const svtkm::Id2& GetPointDimensions() const { return this->PointDimensions; }

  SVTKM_EXEC_CONT
  svtkm::Id2 GetCellDimensions() const { return this->PointDimensions - svtkm::Id2(1); }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfPoints() const { return svtkm::ReduceProduct(this->GetPointDimensions()); }

  //returns an id2 to signal what kind of scheduling to use
  SVTKM_EXEC_CONT
  svtkm::Id2 GetSchedulingRange(svtkm::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  SVTKM_EXEC_CONT
  svtkm::Id2 GetSchedulingRange(svtkm::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  SVTKM_EXEC_CONT
  const svtkm::Id2& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr svtkm::IdComponent NUM_POINTS_IN_CELL = 4;
  static constexpr svtkm::IdComponent MAX_CELL_TO_POINT = 4;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfCells() const { return svtkm::ReduceProduct(this->GetCellDimensions()); }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetCellShape() const { return svtkm::CELL_SHAPE_QUAD; }

  using CellShapeTag = svtkm::CellShapeTagQuad;

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(
    const SchedulingRangeType& logicalCellIndex) const
  {
    svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = this->LogicalToFlatPointIndex(logicalCellIndex);
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    return pointIds;
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(svtkm::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfCellsOnPoint(const SchedulingRangeType& ij) const
  {
    svtkm::IdComponent numCells = 1;

    for (svtkm::IdComponent dim = 0; dim < 2; dim++)
    {
      if ((ij[dim] > 0) && (ij[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfCellsOnPoint(svtkm::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  SVTKM_EXEC_CONT
  svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ij) const
  {
    svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ij[0] > 0) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - svtkm::Id2(1, 1)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - svtkm::Id2(0, 1)));
    }
    if ((ij[0] > 0) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij - svtkm::Id2(1, 0)));
    }
    if ((ij[0] < this->PointDimensions[0] - 1) && (ij[1] < this->PointDimensions[1] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ij));
    }

    return cellIds;
  }

  SVTKM_EXEC_CONT
  svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(svtkm::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  SVTKM_EXEC_CONT
  svtkm::Id2 FlatToLogicalPointIndex(svtkm::Id flatPointIndex) const
  {
    svtkm::Id2 logicalPointIndex;
    logicalPointIndex[0] = flatPointIndex % this->PointDimensions[0];
    logicalPointIndex[1] = flatPointIndex / this->PointDimensions[0];
    return logicalPointIndex;
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatPointIndex(const svtkm::Id2& logicalPointIndex) const
  {
    return logicalPointIndex[0] + this->PointDimensions[0] * logicalPointIndex[1];
  }

  SVTKM_EXEC_CONT
  svtkm::Id2 FlatToLogicalCellIndex(svtkm::Id flatCellIndex) const
  {
    svtkm::Id2 cellDimensions = this->GetCellDimensions();
    svtkm::Id2 logicalCellIndex;
    logicalCellIndex[0] = flatCellIndex % cellDimensions[0];
    logicalCellIndex[1] = flatCellIndex / cellDimensions[0];
    return logicalCellIndex;
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatCellIndex(const svtkm::Id2& logicalCellIndex) const
  {
    svtkm::Id2 cellDimensions = this->GetCellDimensions();
    return logicalCellIndex[0] + cellDimensions[0] * logicalCellIndex[1];
  }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<2> ";
    out << "pointDim[" << this->PointDimensions[0] << " " << this->PointDimensions[1] << "] ";
    out << std::endl;
  }

private:
  svtkm::Id2 PointDimensions = { 0, 0 };
  svtkm::Id2 GlobalPointIndexStart = { 0, 0 };
};

//3 D specialization.
template <>
class ConnectivityStructuredInternals<3>
{
public:
  using SchedulingRangeType = svtkm::Id3;

  SVTKM_EXEC_CONT
  void SetPointDimensions(svtkm::Id3 dims)
  {
    this->PointDimensions = dims;
    this->CellDimensions = dims - svtkm::Id3(1);
    this->CellDim01 = (dims[0] - 1) * (dims[1] - 1);
  }

  SVTKM_EXEC_CONT
  void SetGlobalPointIndexStart(svtkm::Id3 start) { this->GlobalPointIndexStart = start; }

  SVTKM_EXEC_CONT
  const svtkm::Id3& GetPointDimensions() const { return this->PointDimensions; }

  SVTKM_EXEC_CONT
  const svtkm::Id3& GetCellDimensions() const { return this->CellDimensions; }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfPoints() const { return svtkm::ReduceProduct(this->PointDimensions); }

  //returns an id3 to signal what kind of scheduling to use
  SVTKM_EXEC_CONT
  const svtkm::Id3& GetSchedulingRange(svtkm::TopologyElementTagCell) const
  {
    return this->GetCellDimensions();
  }
  SVTKM_EXEC_CONT
  const svtkm::Id3& GetSchedulingRange(svtkm::TopologyElementTagPoint) const
  {
    return this->GetPointDimensions();
  }

  SVTKM_EXEC_CONT
  const svtkm::Id3& GetGlobalPointIndexStart() const { return this->GlobalPointIndexStart; }

  static constexpr svtkm::IdComponent NUM_POINTS_IN_CELL = 8;
  static constexpr svtkm::IdComponent MAX_CELL_TO_POINT = 8;

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfCells() const { return svtkm::ReduceProduct(this->GetCellDimensions()); }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfPointsInCell() const { return NUM_POINTS_IN_CELL; }
  SVTKM_EXEC_CONT
  svtkm::IdComponent GetCellShape() const { return svtkm::CELL_SHAPE_HEXAHEDRON; }

  using CellShapeTag = svtkm::CellShapeTagHexahedron;

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(const SchedulingRangeType& ijk) const
  {
    svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> pointIds;
    pointIds[0] = (ijk[2] * this->PointDimensions[1] + ijk[1]) * this->PointDimensions[0] + ijk[0];
    pointIds[1] = pointIds[0] + 1;
    pointIds[2] = pointIds[1] + this->PointDimensions[0];
    pointIds[3] = pointIds[2] - 1;
    pointIds[4] = pointIds[0] + this->PointDimensions[0] * this->PointDimensions[1];
    pointIds[5] = pointIds[4] + 1;
    pointIds[6] = pointIds[5] + this->PointDimensions[0];
    pointIds[7] = pointIds[6] - 1;

    return pointIds;
  }

  SVTKM_EXEC_CONT
  svtkm::Vec<svtkm::Id, NUM_POINTS_IN_CELL> GetPointsOfCell(svtkm::Id cellIndex) const
  {
    return this->GetPointsOfCell(this->FlatToLogicalCellIndex(cellIndex));
  }

  SVTKM_EXEC_CONT svtkm::IdComponent GetNumberOfCellsOnPoint(const SchedulingRangeType& ijk) const
  {
    svtkm::IdComponent numCells = 1;

    for (svtkm::IdComponent dim = 0; dim < 3; dim++)
    {
      if ((ijk[dim] > 0) && (ijk[dim] < this->PointDimensions[dim] - 1))
      {
        numCells *= 2;
      }
    }

    return numCells;
  }

  SVTKM_EXEC_CONT
  svtkm::IdComponent GetNumberOfCellsOnPoint(svtkm::Id pointIndex) const
  {
    return this->GetNumberOfCellsOnPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  SVTKM_EXEC_CONT
  svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(
    const SchedulingRangeType& ijk) const
  {
    svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> cellIds;

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(1, 1, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(0, 1, 1)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) && (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(1, 0, 1)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] > 0))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(0, 0, 1)));
    }

    if ((ijk[0] > 0) && (ijk[1] > 0) && (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(1, 1, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] > 0) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(0, 1, 0)));
    }
    if ((ijk[0] > 0) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk - svtkm::Id3(1, 0, 0)));
    }
    if ((ijk[0] < this->PointDimensions[0] - 1) && (ijk[1] < this->PointDimensions[1] - 1) &&
        (ijk[2] < this->PointDimensions[2] - 1))
    {
      cellIds.Append(this->LogicalToFlatCellIndex(ijk));
    }

    return cellIds;
  }

  SVTKM_EXEC_CONT
  svtkm::VecVariable<svtkm::Id, MAX_CELL_TO_POINT> GetCellsOfPoint(svtkm::Id pointIndex) const
  {
    return this->GetCellsOfPoint(this->FlatToLogicalPointIndex(pointIndex));
  }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    out << "   UniformConnectivity<3> ";
    out << "pointDim[" << this->PointDimensions[0] << " " << this->PointDimensions[1] << " "
        << this->PointDimensions[2] << "] ";
    out << std::endl;
  }

  SVTKM_EXEC_CONT
  svtkm::Id3 FlatToLogicalPointIndex(svtkm::Id flatPointIndex) const
  {
    const svtkm::Id pointDims01 = this->PointDimensions[0] * this->PointDimensions[1];
    const svtkm::Id indexij = flatPointIndex % pointDims01;

    return svtkm::Id3(indexij % this->PointDimensions[0],
                     indexij / this->PointDimensions[0],
                     flatPointIndex / pointDims01);
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatPointIndex(const svtkm::Id3& logicalPointIndex) const
  {
    return logicalPointIndex[0] +
      this->PointDimensions[0] *
      (logicalPointIndex[1] + this->PointDimensions[1] * logicalPointIndex[2]);
  }

  SVTKM_EXEC_CONT
  svtkm::Id3 FlatToLogicalCellIndex(svtkm::Id flatCellIndex) const
  {
    const svtkm::Id indexij = flatCellIndex % this->CellDim01;
    return svtkm::Id3(indexij % this->CellDimensions[0],
                     indexij / this->CellDimensions[0],
                     flatCellIndex / this->CellDim01);
  }

  SVTKM_EXEC_CONT
  svtkm::Id LogicalToFlatCellIndex(const svtkm::Id3& logicalCellIndex) const
  {
    return logicalCellIndex[0] +
      this->CellDimensions[0] *
      (logicalCellIndex[1] + this->CellDimensions[1] * logicalCellIndex[2]);
  }

private:
  svtkm::Id3 PointDimensions = { 0, 0, 0 };
  svtkm::Id3 GlobalPointIndexStart = { 0, 0, 0 };
  svtkm::Id3 CellDimensions = { 0, 0, 0 };
  svtkm::Id CellDim01 = 0;
};

// We may want to generalize this class depending on how ConnectivityExplicit
// eventually handles retrieving cell to point connectivity.

template <typename VisitTopology, typename IncidentTopology, svtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper
{
  // We want an unconditional failure if this unspecialized class ever gets
  // instantiated, because it means someone missed a topology mapping type.
  // We need to create a test which depends on the templated types so
  // it doesn't get picked up without a concrete instantiation.
  SVTKM_STATIC_ASSERT_MSG(sizeof(VisitTopology) == static_cast<size_t>(-1),
                         "Missing Specialization for Topologies");
};

template <svtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<svtkm::TopologyElementTagCell,
                                         svtkm::TopologyElementTagPoint,
                                         Dimension>
{
  using ConnectivityType = svtkm::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = typename ConnectivityType::CellShapeTag;

  using IndicesType = svtkm::Vec<svtkm::Id, ConnectivityType::NUM_POINTS_IN_CELL>;

  SVTKM_EXEC_CONT static svtkm::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfCells();
  }

  template <typename IndexType>
  SVTKM_EXEC_CONT static svtkm::IdComponent GetNumberOfIndices(
    const ConnectivityType& svtkmNotUsed(connectivity),
    const IndexType& svtkmNotUsed(cellIndex))
  {
    return ConnectivityType::NUM_POINTS_IN_CELL;
  }

  template <typename IndexType>
  SVTKM_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                               const IndexType& cellIndex)
  {
    return connectivity.GetPointsOfCell(cellIndex);
  }

  SVTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalFromIndex(const ConnectivityType& connectivity,
                                                 svtkm::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatFromIndex);
  }

  SVTKM_EXEC_CONT
  static svtkm::Id LogicalToFlatFromIndex(const ConnectivityType& connectivity,
                                         const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalFromIndex);
  }

  SVTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalToIndex(const ConnectivityType& connectivity,
                                               svtkm::Id flatToIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatToIndex);
  }

  SVTKM_EXEC_CONT
  static svtkm::Id LogicalToFlatToIndex(const ConnectivityType& connectivity,
                                       const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalToIndex);
  }
};

template <svtkm::IdComponent Dimension>
struct ConnectivityStructuredIndexHelper<svtkm::TopologyElementTagPoint,
                                         svtkm::TopologyElementTagCell,
                                         Dimension>
{
  using ConnectivityType = svtkm::internal::ConnectivityStructuredInternals<Dimension>;
  using LogicalIndexType = typename ConnectivityType::SchedulingRangeType;

  using CellShapeTag = svtkm::CellShapeTagVertex;

  using IndicesType = svtkm::VecVariable<svtkm::Id, ConnectivityType::MAX_CELL_TO_POINT>;

  SVTKM_EXEC_CONT static svtkm::Id GetNumberOfElements(const ConnectivityType& connectivity)
  {
    return connectivity.GetNumberOfPoints();
  }

  template <typename IndexType>
  SVTKM_EXEC_CONT static svtkm::IdComponent GetNumberOfIndices(const ConnectivityType& connectivity,
                                                             const IndexType& pointIndex)
  {
    return connectivity.GetNumberOfCellsOnPoint(pointIndex);
  }

  template <typename IndexType>
  SVTKM_EXEC_CONT static IndicesType GetIndices(const ConnectivityType& connectivity,
                                               const IndexType& pointIndex)
  {
    return connectivity.GetCellsOfPoint(pointIndex);
  }

  SVTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalFromIndex(const ConnectivityType& connectivity,
                                                 svtkm::Id flatFromIndex)
  {
    return connectivity.FlatToLogicalCellIndex(flatFromIndex);
  }

  SVTKM_EXEC_CONT
  static svtkm::Id LogicalToFlatFromIndex(const ConnectivityType& connectivity,
                                         const LogicalIndexType& logicalFromIndex)
  {
    return connectivity.LogicalToFlatCellIndex(logicalFromIndex);
  }

  SVTKM_EXEC_CONT
  static LogicalIndexType FlatToLogicalToIndex(const ConnectivityType& connectivity,
                                               svtkm::Id flatToIndex)
  {
    return connectivity.FlatToLogicalPointIndex(flatToIndex);
  }

  SVTKM_EXEC_CONT
  static svtkm::Id LogicalToFlatToIndex(const ConnectivityType& connectivity,
                                       const LogicalIndexType& logicalToIndex)
  {
    return connectivity.LogicalToFlatPointIndex(logicalToIndex);
  }
};
}
} // namespace svtkm::internal

#endif //svtk_m_internal_ConnectivityStructuredInternals_h
