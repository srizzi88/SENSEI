//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/raytracing/CylinderExtractor.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/rendering/Cylinderizer.h>
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

class CountSegments : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  SVTKM_CONT
  CountSegments() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& segments) const
  {
    if (shapeType.Id == svtkm::CELL_SHAPE_LINE)
      segments = 1;
    else if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
      segments = 3;
    else if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
      segments = 4;
    else
      segments = 0;
  }

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType), svtkm::Id& points) const
  {
    points = 0;
  }

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagQuad svtkmNotUsed(shapeType), svtkm::Id& points) const
  {
    points = 0;
  }
  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagWedge svtkmNotUsed(shapeType), svtkm::Id& points) const
  {
    points = 0;
  }

}; // ClassCountSegments

class Pointify : public svtkm::worklet::WorkletVisitCellsWithPoints
{

public:
  SVTKM_CONT
  Pointify() {}
  typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
  typedef void ExecutionSignature(_2, CellShape, PointIndices, WorkIndex, _3);

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
  SVTKM_EXEC void operator()(const svtkm::Id& svtkmNotUsed(pointOffset),
                            svtkm::CellShapeTagHexahedron svtkmNotUsed(shapeType),
                            const VecType& svtkmNotUsed(cellIndices),
                            const svtkm::Id& svtkmNotUsed(cellId),
                            OutputPortal& svtkmNotUsed(outputIndices)) const

  {
  }

  template <typename VecType, typename OutputPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& pointOffset,
                            svtkm::CellShapeTagGeneric shapeType,
                            const VecType& cellIndices,
                            const svtkm::Id& cellId,
                            OutputPortal& outputIndices) const
  {

    if (shapeType.Id == svtkm::CELL_SHAPE_LINE)
    {
      svtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = cellIndices[0];
      segment[2] = cellIndices[1];
      outputIndices.Set(pointOffset, segment);
    }
    else if (shapeType.Id == svtkm::CELL_SHAPE_TRIANGLE)
    {
      svtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = cellIndices[0];
      segment[2] = cellIndices[1];
      outputIndices.Set(pointOffset, segment);

      segment[1] = cellIndices[1];
      segment[2] = cellIndices[2];
      outputIndices.Set(pointOffset + 1, segment);

      segment[1] = cellIndices[2];
      segment[2] = cellIndices[0];
      outputIndices.Set(pointOffset + 2, segment);
    }
    else if (shapeType.Id == svtkm::CELL_SHAPE_QUAD)
    {
      svtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = cellIndices[0];
      segment[2] = cellIndices[1];
      outputIndices.Set(pointOffset, segment);

      segment[1] = cellIndices[1];
      segment[2] = cellIndices[2];
      outputIndices.Set(pointOffset + 1, segment);

      segment[1] = cellIndices[2];
      segment[2] = cellIndices[3];
      outputIndices.Set(pointOffset + 2, segment);

      segment[1] = cellIndices[3];
      segment[2] = cellIndices[0];
      outputIndices.Set(pointOffset + 3, segment);
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
  SVTKM_EXEC void operator()(const svtkm::Id3& cylId,
                            svtkm::Float32& radius,
                            const ScalarPortalType& scalars) const
  {
    svtkm::Float32 scalar = static_cast<svtkm::Float32>(scalars.Get(cylId[0]));
    svtkm::Float32 t = (scalar - MinValue) * InverseDelta;
    radius = MinRadius + t * RadiusDelta;
  }

}; //class FieldRadius

} //namespace detail


void CylinderExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells,
                                     const svtkm::Float32 radius)
{
  svtkm::Id numOfSegments;
  svtkm::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetUniformRadius(radius);
}

void CylinderExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells,
                                     const svtkm::cont::Field& field,
                                     const svtkm::Float32 minRadius,
                                     const svtkm::Float32 maxRadius)
{
  svtkm::Id numOfSegments;
  svtkm::rendering::Cylinderizer geometrizer;
  geometrizer.Run(cells, this->CylIds, numOfSegments);

  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void CylinderExtractor::SetUniformRadius(const svtkm::Float32 radius)
{
  const svtkm::Id size = this->CylIds.GetNumberOfValues();
  Radii.Allocate(size);

  svtkm::cont::ArrayHandleConstant<svtkm::Float32> radiusHandle(radius, size);
  svtkm::cont::Algorithm::Copy(radiusHandle, Radii);
}

void CylinderExtractor::SetCylinderIdsFromCells(const svtkm::cont::DynamicCellSet& cells)
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
    svtkm::worklet::DispatcherMapTopology<detail::CountSegments>(detail::CountSegments())
      .Invoke(cells, points);

    svtkm::Id totalPoints = 0;
    totalPoints = svtkm::cont::Algorithm::Reduce(points, svtkm::Id(0));

    svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
    svtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    CylIds.Allocate(totalPoints);

    svtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->CylIds);
  }
}

void CylinderExtractor::SetVaryingRadius(const svtkm::Float32 minRadius,
                                         const svtkm::Float32 maxRadius,
                                         const svtkm::cont::Field& field)
{
  svtkm::cont::ArrayHandle<svtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw svtkm::cont::ErrorBadValue("Cylinder Extractor: scalar field must have one component");
  }

  svtkm::Range range = rangeArray.GetPortalConstControl().Get(0);

  Radii.Allocate(this->CylIds.GetNumberOfValues());
  svtkm::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(this->CylIds, this->Radii, field.GetData().ResetTypes(svtkm::TypeListFieldScalar()));
}


svtkm::cont::ArrayHandle<svtkm::Id3> CylinderExtractor::GetCylIds()
{
  return this->CylIds;
}

svtkm::cont::ArrayHandle<svtkm::Float32> CylinderExtractor::GetRadii()
{
  return this->Radii;
}

svtkm::Id CylinderExtractor::GetNumberOfCylinders() const
{
  return this->CylIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
