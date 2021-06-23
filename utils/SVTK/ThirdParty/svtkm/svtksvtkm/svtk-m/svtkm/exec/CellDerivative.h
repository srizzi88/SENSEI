//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_Derivative_h
#define svtk_m_exec_Derivative_h

#include <svtkm/Assert.h>
#include <svtkm/CellShape.h>
#include <svtkm/VecAxisAlignedPointCoordinates.h>
#include <svtkm/VecTraits.h>

#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/FunctorBase.h>

#include <lcl/lcl.h>

namespace svtkm
{
namespace exec
{

//-----------------------------------------------------------------------------
/// \brief Take the derivative (get the gradient) of a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, finds the derivative with respect to each
/// coordinate (i.e. the gradient) at that point. The derivative is not always
/// constant in some "linear" cells.
///
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& pointFieldValues,
  const WorldCoordType& worldCoordinateValues,
  const svtkm::Vec<ParametricCoordType, 3>& parametricCoords,
  svtkm::CellShapeTagGeneric shape,
  const svtkm::exec::FunctorBase& worklet)
{
  svtkm::Vec<typename FieldVecType::ComponentType, 3> result;
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(
      result = CellDerivative(
        pointFieldValues, worldCoordinateValues, parametricCoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to derivative.");
      return svtkm::Vec<typename FieldVecType::ComponentType, 3>();
  }
  return result;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename VtkcCellShapeTag,
          typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivativeImpl(
  VtkcCellShapeTag tag,
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const ParametricCoordType& pcoords,
  const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSERT(field.GetNumberOfComponents() == tag.numberOfPoints());
  SVTKM_ASSERT(wCoords.GetNumberOfComponents() == tag.numberOfPoints());

  using FieldType = typename FieldVecType::ComponentType;

  auto fieldNumComponents = svtkm::VecTraits<FieldType>::GetNumberOfComponents(field[0]);
  svtkm::Vec<FieldType, 3> derivs;
  auto status = lcl::derivative(tag,
                                lcl::makeFieldAccessorNestedSOA(wCoords, 3),
                                lcl::makeFieldAccessorNestedSOA(field, fieldNumComponents),
                                pcoords,
                                derivs[0],
                                derivs[1],
                                derivs[2]);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
    derivs = svtkm::TypeTraits<svtkm::Vec<FieldType, 3>>::ZeroInitialization();
  }

  return derivs;
}

} // namespace internal

template <typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag shape,
  const svtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(
    svtkm::internal::make_VtkcCellShapeTag(shape), field, wCoords, pcoords, worklet);
}

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType&,
  const WorldCoordType&,
  const svtkm::Vec<ParametricCoordType, 3>&,
  svtkm::CellShapeTagEmpty,
  const svtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to take derivative in empty cell.");
  return svtkm::Vec<typename FieldVecType::ComponentType, 3>();
}

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagPolyLine,
  const svtkm::exec::FunctorBase& worklet)
{
  svtkm::IdComponent numPoints = field.GetNumberOfComponents();
  SVTKM_ASSERT(numPoints >= 1);
  SVTKM_ASSERT(numPoints == wCoords.GetNumberOfComponents());

  switch (numPoints)
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, svtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellDerivative(field, wCoords, pcoords, svtkm::CellShapeTagLine(), worklet);
  }

  auto dt = static_cast<ParametricCoordType>(1) / static_cast<ParametricCoordType>(numPoints - 1);
  auto idx = static_cast<svtkm::IdComponent>(svtkm::Ceil(pcoords[0] / dt));
  if (idx == 0)
  {
    idx = 1;
  }
  if (idx > numPoints - 1)
  {
    idx = numPoints - 1;
  }

  auto lineField = svtkm::make_Vec(field[idx - 1], field[idx]);
  auto lineWCoords = svtkm::make_Vec(wCoords[idx - 1], wCoords[idx]);
  auto pc = (pcoords[0] - static_cast<ParametricCoordType>(idx) * dt) / dt;
  return internal::CellDerivativeImpl(lcl::Line{}, lineField, lineWCoords, &pc, worklet);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagPolygon,
  const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSERT(field.GetNumberOfComponents() == wCoords.GetNumberOfComponents());

  const svtkm::IdComponent numPoints = field.GetNumberOfComponents();
  SVTKM_ASSERT(numPoints > 0);

  switch (field.GetNumberOfComponents())
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, svtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellDerivative(field, wCoords, pcoords, svtkm::CellShapeTagLine(), worklet);
    default:
      return internal::CellDerivativeImpl(
        lcl::Polygon(numPoints), field, wCoords, pcoords, worklet);
  }
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const svtkm::VecAxisAlignedPointCoordinates<2>& wCoords,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagQuad,
  const svtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(lcl::Pixel{}, field, wCoords, pcoords, worklet);
}

template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec<typename FieldVecType::ComponentType, 3> CellDerivative(
  const FieldVecType& field,
  const svtkm::VecAxisAlignedPointCoordinates<3>& wCoords,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagHexahedron,
  const svtkm::exec::FunctorBase& worklet)
{
  return internal::CellDerivativeImpl(lcl::Voxel{}, field, wCoords, pcoords, worklet);
}
}
} // namespace svtkm::exec

#endif //svtk_m_exec_Derivative_h
