//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_Interpolate_h
#define svtk_m_exec_Interpolate_h

#include <svtkm/Assert.h>
#include <svtkm/CellShape.h>
#include <svtkm/VecAxisAlignedPointCoordinates.h>
#include <svtkm/exec/FunctorBase.h>

#include <lcl/lcl.h>

#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

namespace svtkm
{
namespace exec
{

namespace internal
{

template <typename VtkcCellShapeTag, typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolateImpl(
  VtkcCellShapeTag tag,
  const FieldVecType& field,
  const ParametricCoordType& pcoords,
  const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSERT(tag.numberOfPoints() == field.GetNumberOfComponents());

  using FieldValueType = typename FieldVecType::ComponentType;
  IdComponent numComponents = svtkm::VecTraits<FieldValueType>::GetNumberOfComponents(field[0]);
  FieldValueType result(0);
  auto status =
    lcl::interpolate(tag, lcl::makeFieldAccessorNestedSOA(field, numComponents), pcoords, result);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
  }
  return result;
}

} // namespace internal

//-----------------------------------------------------------------------------
/// \brief Interpolate a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, interpolates the field to that point.
///
template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const svtkm::Vec<ParametricCoordType, 3>& parametricCoords,
  svtkm::CellShapeTagGeneric shape,
  const svtkm::exec::FunctorBase& worklet)
{
  typename FieldVecType::ComponentType result;
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(
      result = CellInterpolate(pointFieldValues, parametricCoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Unknown cell shape sent to interpolate.");
      return typename FieldVecType::ComponentType();
  }
  return result;
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType, typename CellShapeTag>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& pointFieldValues,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag tag,
  const svtkm::exec::FunctorBase& worklet)
{
  auto lclTag =
    svtkm::internal::make_VtkcCellShapeTag(tag, pointFieldValues.GetNumberOfComponents());
  return internal::CellInterpolateImpl(lclTag, pointFieldValues, pcoords, worklet);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType&,
  const svtkm::Vec<ParametricCoordType, 3>&,
  svtkm::CellShapeTagEmpty,
  const svtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to interpolate an empty cell.");
  return typename FieldVecType::ComponentType();
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagPolyLine,
  const svtkm::exec::FunctorBase& worklet)
{
  const svtkm::IdComponent numPoints = field.GetNumberOfComponents();
  SVTKM_ASSERT(numPoints >= 1);

  if (numPoints == 1)
  {
    return CellInterpolate(field, pcoords, svtkm::CellShapeTagVertex(), worklet);
  }

  using T = ParametricCoordType;

  T dt = 1 / static_cast<T>(numPoints - 1);
  svtkm::IdComponent idx = static_cast<svtkm::IdComponent>(pcoords[0] / dt);
  if (idx == numPoints - 1)
  {
    return field[numPoints - 1];
  }

  T pc = (pcoords[0] - static_cast<T>(idx) * dt) / dt;
  return internal::CellInterpolateImpl(
    lcl::Line{}, svtkm::make_Vec(field[idx], field[idx + 1]), &pc, worklet);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
SVTKM_EXEC typename FieldVecType::ComponentType CellInterpolate(
  const FieldVecType& field,
  const svtkm::Vec<ParametricCoordType, 3>& pcoords,
  svtkm::CellShapeTagPolygon,
  const svtkm::exec::FunctorBase& worklet)
{
  const svtkm::IdComponent numPoints = field.GetNumberOfComponents();
  SVTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      return CellInterpolate(field, pcoords, svtkm::CellShapeTagVertex(), worklet);
    case 2:
      return CellInterpolate(field, pcoords, svtkm::CellShapeTagLine(), worklet);
    default:
      return internal::CellInterpolateImpl(lcl::Polygon(numPoints), field, pcoords, worklet);
  }
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec3f CellInterpolate(const svtkm::VecAxisAlignedPointCoordinates<2>& field,
                                      const svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                      svtkm::CellShapeTagQuad,
                                      const svtkm::exec::FunctorBase& worklet)
{
  return internal::CellInterpolateImpl(lcl::Pixel{}, field, pcoords, worklet);
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
SVTKM_EXEC svtkm::Vec3f CellInterpolate(const svtkm::VecAxisAlignedPointCoordinates<3>& field,
                                      const svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                      svtkm::CellShapeTagHexahedron,
                                      const svtkm::exec::FunctorBase& worklet)
{
  return internal::CellInterpolateImpl(lcl::Voxel{}, field, pcoords, worklet);
}
}
} // namespace svtkm::exec

#if (defined(SVTKM_GCC) || defined(SVTKM_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

#endif //svtk_m_exec_Interpolate_h
