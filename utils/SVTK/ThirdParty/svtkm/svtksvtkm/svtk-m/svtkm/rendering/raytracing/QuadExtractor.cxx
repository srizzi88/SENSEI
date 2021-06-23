//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/raytracing/QuadExtractor.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/rendering/Quadralizer.h>
#include <svtkm/rendering/raytracing/Worklets.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class CountQuads : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  SVTKM_CONT
  CountQuads() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& quads) const
  {
    if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      quads = 1;
    else
      quads = 0;
  }

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType), svtkm::Id& quads) const
  {
    quads = 6;
  }

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagQuad svtkmNotUsed(shapeType), svtkm::Id& points) const
  {
    points = 1;
  }
  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagWedge svtkmNotUsed(shapeType), svtkm::Id& points) const
  {
    points = 0;
  }

}; // ClassCountquads

class Pointify : public svtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  SVTKM_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void cell2quad(svtkm::Id& offset,
                           const VecType& cellIndices,
                           const svtkm::Id& cellId,
                           const svtkm::Id Id0,
                           const svtkm::Id Id1,
                           const svtkm::Id Id2,
                           const svtkm::Id Id3,
                           OutputPortal& outputIndices) const
  {
    svtkm::Vec<svtkm::Id, 5> quad;
    quad[0] = cellId;
    quad[1] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id0)]);
    quad[2] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id1)]);
    quad[3] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id2)]);
    quad[4] = static_cast<svtkm::Id>(cellIndices[svtkm::IdComponent(Id3)]);
    outputIndices.Set(offset++, quad);
  }

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& svtkmNotUsed(pointOffset),
                            svtkm::CellShapeTagQuad svtkmNotUsed(shapeType),
                            const VecType& svtkmNotUsed(cellIndices),
                            const svtkm::Id& svtkmNotUsed(cellId),
                            OutputPortal& svtkmNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& svtkmNotUsed(pointOffset),
                            svtkm::CellShapeTagWedge svtkmNotUsed(shapeType),
                            const VecType& svtkmNotUsed(cellIndices),
                            const svtkm::Id& svtkmNotUsed(cellId),
                            OutputPortal& svtkmNotUsed(outputIndices)) const
  {
  }

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                            svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType),
                            const VecType& cellIndices,
                            const svtkm::Id& cellId,
                            OutputPortal& outputIndices) const

  {
    svtkm::Id offset = pointOffset;
    cell2quad(offset, cellIndices, cellId, 0, 1, 5, 4, outputIndices);
    cell2quad(offset, cellIndices, cellId, 1, 2, 6, 5, outputIndices);
    cell2quad(offset, cellIndices, cellId, 3, 7, 6, 2, outputIndices);
    cell2quad(offset, cellIndices, cellId, 0, 4, 7, 3, outputIndices);
    cell2quad(offset, cellIndices, cellId, 0, 3, 2, 1, outputIndices);
    cell2quad(offset, cellIndices, cellId, 4, 5, 6, 7, outputIndices);
  }

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                            svtkm::CellShapeTagGeneric shapeType,
                            const VecType& cellIndices,
                            const svtkm::Id& cellId,
                            OutputPortal& outputIndices) const
  {

    if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
    {
      svtkm::Vec<svtkm::Id, 5> quad;
      quad[0] = cellId;
      quad[1] = cellIndices[0];
      quad[2] = cellIndices[1];
      quad[3] = cellIndices[2];
      quad[4] = cellIndices[3];
      outputIndices.Set(pointOffset, quad);
    }
  }
}; //class pointify

class Iterator : public svtkm::worklet::WorkletMapField
{

public:
  SVTKM_CONT
  Iterator() {}
  typedef void ControlSignature(FieldOut);
  typedef void ExecutionSignature(_1, WorkIndex);
  SVTKM_EXEC
  void operator()(svtkm::Id2& index, const svtkm::Id2& idx) const { index = idx; }
}; //class Iterator

class FieldRadius : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Float32 MinRadius;
  svtkm::Float32 RadiusDelta;
  svtkm::Float32 MinValue;
  svtkm::Float32 InverseDelta;

public:
  SVTKM_CONT
  FieldRadius(const svtkm::Float32 minRadius,
              const svtkm::Float32 maxRadius,
              const svtkm::Range scalarRange)
    : MinRadius(minRadius)
    , RadiusDelta(maxRadius - minRadius)
    , MinValue(static_cast<svtkm::Float32>(scalarRange.Min))
  {
    svtkm::Float32 delta = static_cast<svtkm::Float32>(scalarRange.Max - scalarRange.Min);
    if (delta != 0.f)
      InverseDelta = 1.f / (delta);
    else
      InverseDelta = 0.f; // just map scalar to 0;
  }

  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename ScalarPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& pointId,
                            svtkm::Float32& radius,
                            const ScalarPortalType& scalars) const
  {
    svtkm::Float32 scalar = static_cast<svtkm::Float32>(scalars.Get(pointId));
    svtkm::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail

void QuadExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells)
{
  svtkm::Id numOfQuads;
  svtkm::rendering::Quadralizer quadrizer;
  quadrizer.Run(cells, this->QuadIds, numOfQuads);

  //this->SetPointIdsFromCells(cells);
}


void QuadExtractor::SetQuadIdsFromCells(const svtkm::cont::DynamicCellSet& cells)
{
  svtkm::Id numCells = cells.GetNumberOfCells();
  if (numCells == 0)
  {
    return;
  }
  //
  // look for points in the cell set
  //
  if (cells.IsSameType(svtkm::cont::CellSetExplicit<>()))
  {
    svtkm::cont::ArrayHandle<svtkm::Id> points;
    svtkm::worklet::DispatcherMapTopology<detail::CountQuads>(detail::CountQuads())
      .Invoke(cells, points);

    svtkm::Id total = 0;
    total = svtkm::cont::Algorithm::Reduce(points, svtkm::Id(0));

    svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
    svtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    QuadIds.Allocate(total);

    svtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->QuadIds);
  }
}


svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>> QuadExtractor::GetQuadIds()
{
  return this->QuadIds;
}


svtkm::Id QuadExtractor::GetNumberOfQuads() const
{
  return this->QuadIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
