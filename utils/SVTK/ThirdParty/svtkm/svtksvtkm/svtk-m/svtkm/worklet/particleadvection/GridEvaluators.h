//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_particleadvection_GridEvaluators_h
#define svtk_m_worklet_particleadvection_GridEvaluators_h

#include <svtkm/Bitset.h>
#include <svtkm/Types.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/CellLocatorRectilinearGrid.h>
#include <svtkm/cont/CellLocatorUniformBins.h>
#include <svtkm/cont/CellLocatorUniformGrid.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/worklet/particleadvection/CellInterpolationHelper.h>
#include <svtkm/worklet/particleadvection/GridEvaluatorStatus.h>
#include <svtkm/worklet/particleadvection/Integrators.h>

namespace svtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionGridEvaluator
{
  using FieldPortalType =
    typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

public:
  SVTKM_CONT
  ExecutionGridEvaluator() = default;

  SVTKM_CONT
  ExecutionGridEvaluator(std::shared_ptr<svtkm::cont::CellLocator> locator,
                         std::shared_ptr<svtkm::cont::CellInterpolationHelper> interpolationHelper,
                         const svtkm::Bounds& bounds,
                         const FieldArrayType& field)
    : Bounds(bounds)
    , Field(field.PrepareForInput(DeviceAdapter()))
  {
    Locator = locator->PrepareForExecution(DeviceAdapter());
    InterpolationHelper = interpolationHelper->PrepareForExecution(DeviceAdapter());
  }

  template <typename Point>
  SVTKM_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    svtkm::Id cellId;
    Point parametric;
    svtkm::exec::FunctorBase tmp;

    Locator->FindCell(point, cellId, parametric, tmp);
    return cellId != -1;
  }

  SVTKM_EXEC
  bool IsWithinTemporalBoundary(const svtkm::FloatDefault svtkmNotUsed(time)) const { return true; }

  SVTKM_EXEC
  svtkm::Bounds GetSpatialBoundary() const { return this->Bounds; }

  SVTKM_EXEC_CONT
  svtkm::FloatDefault GetTemporalBoundary(svtkm::Id direction) const
  {
    // Return the time of the newest time slice
    return direction > 0 ? svtkm::Infinity<svtkm::FloatDefault>()
                         : svtkm::NegativeInfinity<svtkm::FloatDefault>();
  }

  template <typename Point>
  SVTKM_EXEC GridEvaluatorStatus Evaluate(const Point& pos,
                                         svtkm::FloatDefault svtkmNotUsed(time),
                                         Point& out) const
  {
    return this->Evaluate(pos, out);
  }

  template <typename Point>
  SVTKM_EXEC GridEvaluatorStatus Evaluate(const Point& point, Point& out) const
  {
    svtkm::Id cellId;
    Point parametric;
    svtkm::exec::FunctorBase tmp;
    GridEvaluatorStatus status;

    Locator->FindCell(point, cellId, parametric, tmp);
    if (cellId == -1)
    {
      status.SetFail();
      status.SetSpatialBounds();
      return status;
    }

    svtkm::UInt8 cellShape;
    svtkm::IdComponent nVerts;
    svtkm::VecVariable<svtkm::Id, 8> ptIndices;
    svtkm::VecVariable<svtkm::Vec3f, 8> fieldValues;
    InterpolationHelper->GetCellInfo(cellId, cellShape, nVerts, ptIndices);

    for (svtkm::IdComponent i = 0; i < nVerts; i++)
      fieldValues.Append(Field.Get(ptIndices[i]));
    out = svtkm::exec::CellInterpolate(fieldValues, parametric, cellShape, tmp);

    status.SetOk();
    return status;
  }

private:
  const svtkm::exec::CellLocator* Locator;
  const svtkm::exec::CellInterpolationHelper* InterpolationHelper;
  svtkm::Bounds Bounds;
  FieldPortalType Field;
};

template <typename FieldArrayType>
class GridEvaluator : public svtkm::cont::ExecutionObjectBase
{
public:
  using UniformType = svtkm::cont::ArrayHandleUniformPointCoordinates;
  using AxisHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
  using RectilinearType =
    svtkm::cont::ArrayHandleCartesianProduct<AxisHandle, AxisHandle, AxisHandle>;
  using Structured2DType = svtkm::cont::CellSetStructured<2>;
  using Structured3DType = svtkm::cont::CellSetStructured<3>;

  SVTKM_CONT
  GridEvaluator() = default;

  SVTKM_CONT
  GridEvaluator(const svtkm::cont::CoordinateSystem& coordinates,
                const svtkm::cont::DynamicCellSet& cellset,
                const FieldArrayType& field)
    : Vectors(field)
    , Bounds(coordinates.GetBounds())
  {
    if (cellset.IsSameType(Structured2DType()) || cellset.IsSameType(Structured3DType()))
    {
      if (coordinates.GetData().IsType<UniformType>())
      {
        svtkm::cont::CellLocatorUniformGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<svtkm::cont::CellLocatorUniformGrid>(locator);
      }
      else if (coordinates.GetData().IsType<RectilinearType>())
      {
        svtkm::cont::CellLocatorRectilinearGrid locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<svtkm::cont::CellLocatorRectilinearGrid>(locator);
      }
      else
      {
        // Default to using an locator for explicit meshes.
        svtkm::cont::CellLocatorUniformBins locator;
        locator.SetCoordinates(coordinates);
        locator.SetCellSet(cellset);
        locator.Update();
        this->Locator = std::make_shared<svtkm::cont::CellLocatorUniformBins>(locator);
      }
      svtkm::cont::StructuredCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<svtkm::cont::StructuredCellInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(svtkm::cont::CellSetSingleType<>()))
    {
      svtkm::cont::CellLocatorUniformBins locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<svtkm::cont::CellLocatorUniformBins>(locator);
      svtkm::cont::SingleCellTypeInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<svtkm::cont::SingleCellTypeInterpolationHelper>(interpolationHelper);
    }
    else if (cellset.IsSameType(svtkm::cont::CellSetExplicit<>()))
    {
      svtkm::cont::CellLocatorUniformBins locator;
      locator.SetCoordinates(coordinates);
      locator.SetCellSet(cellset);
      locator.Update();
      this->Locator = std::make_shared<svtkm::cont::CellLocatorUniformBins>(locator);
      svtkm::cont::ExplicitCellInterpolationHelper interpolationHelper(cellset);
      this->InterpolationHelper =
        std::make_shared<svtkm::cont::ExplicitCellInterpolationHelper>(interpolationHelper);
    }
    else
      throw svtkm::cont::ErrorInternal("Unsupported cellset type.");
  }

  template <typename DeviceAdapter>
  SVTKM_CONT ExecutionGridEvaluator<DeviceAdapter, FieldArrayType> PrepareForExecution(
    DeviceAdapter) const
  {
    return ExecutionGridEvaluator<DeviceAdapter, FieldArrayType>(
      this->Locator, this->InterpolationHelper, this->Bounds, this->Vectors);
  }

private:
  std::shared_ptr<svtkm::cont::CellLocator> Locator;
  std::shared_ptr<svtkm::cont::CellInterpolationHelper> InterpolationHelper;
  FieldArrayType Vectors;
  svtkm::Bounds Bounds;
};

} //namespace particleadvection
} //namespace worklet
} //namespace svtkm

#endif // svtk_m_worklet_particleadvection_GridEvaluators_h
