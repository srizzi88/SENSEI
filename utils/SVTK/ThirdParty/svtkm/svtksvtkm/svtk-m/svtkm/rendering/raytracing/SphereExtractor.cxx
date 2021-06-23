//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/raytracing/SphereExtractor.h>

#include <svtkm/cont/Algorithm.h>
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

class CountPoints : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  SVTKM_CONT
  CountPoints() {}
  typedef void ControlSignature(CellSetIn cellset, FieldOut);
  typedef void ExecutionSignature(CellShape, _2);

  SVTKM_EXEC
  void operator()(svtkm::CellShapeTagGeneric shapeType, svtkm::Id& points) const
  {
    if (shapeType.Id == svtkm::CELL_SHAPE_VERTEX)
      points = 1;
    else
      points = 0;
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

}; // ClassCountPoints

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
                            const VecType& svtkmNotUsed(cellIndices),
                            const svtkm::Id& cellId,
                            OutputPortal& outputIndices) const
  {

    if (shapeType.Id == svtkm::CELL_SHAPE_VERTEX)
    {
      outputIndices.Set(pointOffset, cellId);
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
  void operator()(svtkm::Id& index, const svtkm::Id& idx) const { index = idx; }
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

void SphereExtractor::ExtractCoordinates(const svtkm::cont::CoordinateSystem& coords,
                                         const svtkm::Float32 radius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetUniformRadius(radius);
}

void SphereExtractor::ExtractCoordinates(const svtkm::cont::CoordinateSystem& coords,
                                         const svtkm::cont::Field& field,
                                         const svtkm::Float32 minRadius,
                                         const svtkm::Float32 maxRadius)
{
  this->SetPointIdsFromCoords(coords);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells,
                                   const svtkm::Float32 radius)
{
  this->SetPointIdsFromCells(cells);
  this->SetUniformRadius(radius);
}
void SphereExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells,
                                   const svtkm::cont::Field& field,
                                   const svtkm::Float32 minRadius,
                                   const svtkm::Float32 maxRadius)
{
  this->SetPointIdsFromCells(cells);
  this->SetVaryingRadius(minRadius, maxRadius, field);
}

void SphereExtractor::SetUniformRadius(const svtkm::Float32 radius)
{
  const svtkm::Id size = this->PointIds.GetNumberOfValues();
  Radii.Allocate(size);

  svtkm::cont::ArrayHandleConstant<svtkm::Float32> radiusHandle(radius, size);
  svtkm::cont::Algorithm::Copy(radiusHandle, Radii);
}

void SphereExtractor::SetPointIdsFromCoords(const svtkm::cont::CoordinateSystem& coords)
{
  svtkm::Id size = coords.GetNumberOfPoints();
  this->PointIds.Allocate(size);
  svtkm::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator()).Invoke(this->PointIds);
}

void SphereExtractor::SetPointIdsFromCells(const svtkm::cont::DynamicCellSet& cells)
{
  using SingleType = svtkm::cont::CellSetSingleType<>;
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
    svtkm::worklet::DispatcherMapTopology<detail::CountPoints>(detail::CountPoints())
      .Invoke(cells, points);

    svtkm::Id totalPoints = 0;
    totalPoints = svtkm::cont::Algorithm::Reduce(points, svtkm::Id(0));

    svtkm::cont::ArrayHandle<svtkm::Id> cellOffsets;
    svtkm::cont::Algorithm::ScanExclusive(points, cellOffsets);
    PointIds.Allocate(totalPoints);

    svtkm::worklet::DispatcherMapTopology<detail::Pointify>(detail::Pointify())
      .Invoke(cells, cellOffsets, this->PointIds);
  }
  else if (cells.IsSameType(SingleType()))
  {
    SingleType pointCells = cells.Cast<SingleType>();
    svtkm::UInt8 shape_id = pointCells.GetCellShape(0);
    if (shape_id == svtkm::CELL_SHAPE_VERTEX)
    {
      this->PointIds.Allocate(numCells);
      svtkm::worklet::DispatcherMapField<detail::Iterator>(detail::Iterator())
        .Invoke(this->PointIds);
    }
  }
}

void SphereExtractor::SetVaryingRadius(const svtkm::Float32 minRadius,
                                       const svtkm::Float32 maxRadius,
                                       const svtkm::cont::Field& field)
{
  svtkm::cont::ArrayHandle<svtkm::Range> rangeArray = field.GetRange();
  if (rangeArray.GetNumberOfValues() != 1)
  {
    throw svtkm::cont::ErrorBadValue("Sphere Extractor: scalar field must have one component");
  }

  svtkm::Range range = rangeArray.GetPortalConstControl().Get(0);

  Radii.Allocate(this->PointIds.GetNumberOfValues());
  svtkm::worklet::DispatcherMapField<detail::FieldRadius>(
    detail::FieldRadius(minRadius, maxRadius, range))
    .Invoke(this->PointIds, this->Radii, field.GetData().ResetTypes(svtkm::TypeListFieldScalar()));
}

svtkm::cont::ArrayHandle<svtkm::Id> SphereExtractor::GetPointIds()
{
  return this->PointIds;
}

svtkm::cont::ArrayHandle<svtkm::Float32> SphereExtractor::GetRadii()
{
  return this->Radii;
}

svtkm::Id SphereExtractor::GetNumberOfSpheres() const
{
  return this->PointIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
