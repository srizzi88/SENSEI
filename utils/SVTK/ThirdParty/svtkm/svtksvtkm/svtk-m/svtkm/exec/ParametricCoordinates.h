//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_ParametricCoordinates_h
#define svtk_m_exec_ParametricCoordinates_h

#include <svtkm/Assert.h>
#include <svtkm/CellShape.h>
#include <svtkm/VecAxisAlignedPointCoordinates.h>
#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/internal/FastVec.h>
#include <svtkm/internal/Assume.h>

#include <lcl/lcl.h>

namespace svtkm
{
namespace exec
{

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         CellShapeTag,
                                                         const svtkm::exec::FunctorBase&)
{
  auto lclTag = typename svtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type{};

  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSERT(numPoints == lclTag.numberOfPoints());

  pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  lcl::parametricCenter(lclTag, pcoords);
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         svtkm::CellShapeTagEmpty,
                                                         const svtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSERT(numPoints == 0);
  pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         svtkm::CellShapeTagVertex,
                                                         const svtkm::exec::FunctorBase&)
{
  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSERT(numPoints == 1);
  pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         svtkm::CellShapeTagPolyLine,
                                                         const svtkm::exec::FunctorBase& worklet)
{
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(numPoints, pcoords, svtkm::CellShapeTagVertex(), worklet);
      return;
    case 2:
      ParametricCoordinatesCenter(numPoints, pcoords, svtkm::CellShapeTagLine(), worklet);
      return;
  }
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         svtkm::CellShapeTagPolygon,
                                                         const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSERT(numPoints > 0);
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesCenter(numPoints, pcoords, svtkm::CellShapeTagVertex(), worklet);
      break;
    case 2:
      ParametricCoordinatesCenter(numPoints, pcoords, svtkm::CellShapeTagLine(), worklet);
      break;
    default:
      pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      lcl::parametricCenter(lcl::Polygon(numPoints), pcoords);
      break;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesCenter(svtkm::IdComponent numPoints,
                                                         svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                         svtkm::CellShapeTagGeneric shape,
                                                         const svtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(
      ParametricCoordinatesCenter(numPoints, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesCenter.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::Vec3f ParametricCoordinatesCenter(
  svtkm::IdComponent numPoints,
  CellShapeTag shape,
  const svtkm::exec::FunctorBase& worklet)
{
  svtkm::Vec3f pcoords(0.0f);
  ParametricCoordinatesCenter(numPoints, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent numPoints,
                                                        svtkm::IdComponent pointIndex,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        CellShapeTag,
                                                        const svtkm::exec::FunctorBase&)
{
  auto lclTag = typename svtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type{};

  (void)numPoints; // Silence compiler warnings.
  SVTKM_ASSUME(numPoints == lclTag.numberOfPoints());
  SVTKM_ASSUME((pointIndex >= 0) && (pointIndex < numPoints));

  pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  lcl::parametricPoint(lclTag, pointIndex, pcoords);
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent,
                                                        svtkm::IdComponent,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        svtkm::CellShapeTagEmpty,
                                                        const svtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Empty cell has no points.");
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent numPoints,
                                                        svtkm::IdComponent pointIndex,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        svtkm::CellShapeTagVertex,
                                                        const svtkm::exec::FunctorBase&)
{
  (void)numPoints;  // Silence compiler warnings.
  (void)pointIndex; // Silence compiler warnings.
  SVTKM_ASSUME(numPoints == 1);
  SVTKM_ASSUME(pointIndex == 0);
  pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent numPoints,
                                                        svtkm::IdComponent pointIndex,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        svtkm::CellShapeTagPolyLine,
                                                        const svtkm::exec::FunctorBase& functor)
{
  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, svtkm::CellShapeTagVertex(), functor);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, svtkm::CellShapeTagLine(), functor);
      return;
  }
  pcoords[0] =
    static_cast<ParametricCoordType>(pointIndex) / static_cast<ParametricCoordType>(numPoints - 1);
  pcoords[1] = 0;
  pcoords[2] = 0;
}

template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent numPoints,
                                                        svtkm::IdComponent pointIndex,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        svtkm::CellShapeTagPolygon,
                                                        const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSUME((numPoints > 0));
  SVTKM_ASSUME((pointIndex >= 0) && (pointIndex < numPoints));

  switch (numPoints)
  {
    case 1:
      ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, svtkm::CellShapeTagVertex(), worklet);
      return;
    case 2:
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, svtkm::CellShapeTagLine(), worklet);
      return;
    default:
      pcoords = svtkm::TypeTraits<svtkm::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      lcl::parametricPoint(lcl::Polygon(numPoints), pointIndex, pcoords);
      return;
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename ParametricCoordType>
static inline SVTKM_EXEC void ParametricCoordinatesPoint(svtkm::IdComponent numPoints,
                                                        svtkm::IdComponent pointIndex,
                                                        svtkm::Vec<ParametricCoordType, 3>& pcoords,
                                                        svtkm::CellShapeTagGeneric shape,
                                                        const svtkm::exec::FunctorBase& worklet)
{
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(
      ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      break;
  }
}

/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
template <typename CellShapeTag>
static inline SVTKM_EXEC svtkm::Vec3f ParametricCoordinatesPoint(
  svtkm::IdComponent numPoints,
  svtkm::IdComponent pointIndex,
  CellShapeTag shape,
  const svtkm::exec::FunctorBase& worklet)
{
  svtkm::Vec3f pcoords(0.0f);
  ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, worklet);
  return pcoords;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename VtkcCellShapeTag, typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinatesImpl(VtkcCellShapeTag tag,
                                            const WorldCoordVector& pointWCoords,
                                            const PCoordType& pcoords,
                                            const svtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType wcoords(0);
  auto status =
    lcl::parametricToWorld(tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), pcoords, wcoords);
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
  }
  return wcoords;
}

} // namespace internal

template <typename WorldCoordVector, typename PCoordType, typename CellShapeTag>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        CellShapeTag shape,
                                        const svtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    svtkm::internal::make_VtkcCellShapeTag(shape, numPoints), pointWCoords, pcoords, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagEmpty empty,
                                        const svtkm::exec::FunctorBase& worklet)
{
  return svtkm::exec::CellInterpolate(pointWCoords, pcoords, empty, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagPolyLine polyLine,
                                        const svtkm::exec::FunctorBase& worklet)
{
  return svtkm::exec::CellInterpolate(pointWCoords, pcoords, polyLine, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagPolygon,
                                        const svtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, svtkm::CellShapeTagVertex{}, worklet);
    case 2:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, svtkm::CellShapeTagLine{}, worklet);
    default:
      return internal::ParametricCoordinatesToWorldCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, pcoords, worklet);
  }
}

template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const svtkm::VecAxisAlignedPointCoordinates<2>& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagQuad,
                                        const svtkm::exec::FunctorBase& worklet)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, pcoords, worklet);
}

template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const svtkm::VecAxisAlignedPointCoordinates<3>& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagHexahedron,
                                        const svtkm::exec::FunctorBase& worklet)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, pcoords, worklet);
}

//-----------------------------------------------------------------------------
/// Returns the world coordinate corresponding to the given parametric coordinate of a cell.
///
template <typename WorldCoordVector, typename PCoordType>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
ParametricCoordinatesToWorldCoordinates(const WorldCoordVector& pointWCoords,
                                        const svtkm::Vec<PCoordType, 3>& pcoords,
                                        svtkm::CellShapeTagGeneric shape,
                                        const svtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType wcoords(0);
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(wcoords = ParametricCoordinatesToWorldCoordinates(
                                pointWCoords, pcoords, CellShapeTag(), worklet));
    default:
      worklet.RaiseError("Bad shape given to ParametricCoordinatesPoint.");
      break;
  }
  return wcoords;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename VtkcCellShapeTag, typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinatesImpl(VtkcCellShapeTag tag,
                                            const WorldCoordVector& pointWCoords,
                                            const typename WorldCoordVector::ComponentType& wcoords,
                                            bool& success,
                                            const svtkm::exec::FunctorBase& worklet)
{
  SVTKM_ASSERT(pointWCoords.GetNumberOfComponents() == tag.numberOfPoints());

  auto pcoords = svtkm::TypeTraits<typename WorldCoordVector::ComponentType>::ZeroInitialization();
  auto status =
    lcl::worldToParametric(tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), wcoords, pcoords);

  success = true;
  if (status != lcl::ErrorCode::SUCCESS)
  {
    worklet.RaiseError(lcl::errorString(status));
    success = false;
  }
  return pcoords;
}

} // namespace internal

template <typename WorldCoordVector, typename CellShapeTag>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        CellShapeTag shape,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    svtkm::internal::make_VtkcCellShapeTag(shape, numPoints),
    pointWCoords,
    wcoords,
    success,
    worklet);
}

template <typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector&,
                                        const typename WorldCoordVector::ComponentType&,
                                        svtkm::CellShapeTagEmpty,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& worklet)
{
  worklet.RaiseError("Attempted to find point coordinates in empty cell.");
  success = false;
  return typename WorldCoordVector::ComponentType();
}

template <typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType&,
                                        svtkm::CellShapeTagVertex,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& svtkmNotUsed(worklet))
{
  (void)pointWCoords; // Silence compiler warnings.
  SVTKM_ASSERT(pointWCoords.GetNumberOfComponents() == 1);
  success = true;
  return typename WorldCoordVector::ComponentType(0, 0, 0);
}

template <typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        svtkm::CellShapeTagPolyLine,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& worklet)
{
  svtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();
  SVTKM_ASSERT(pointWCoords.GetNumberOfComponents() >= 1);

  if (numPoints == 1)
  {
    return WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, svtkm::CellShapeTagVertex(), success, worklet);
  }

  using Vector3 = typename WorldCoordVector::ComponentType;
  using T = typename Vector3::ComponentType;

  //Find the closest vertex to the point.
  svtkm::IdComponent idx = 0;
  Vector3 vec = pointWCoords[0] - wcoords;
  T minDistSq = svtkm::Dot(vec, vec);
  for (svtkm::IdComponent i = 1; i < numPoints; i++)
  {
    vec = pointWCoords[i] - wcoords;
    T d = svtkm::Dot(vec, vec);

    if (d < minDistSq)
    {
      idx = i;
      minDistSq = d;
    }
  }

  //Find the right segment, and the parameterization along that segment.
  //Closest to 0, so segment is (0,1)
  if (idx == 0)
  {
    idx = 1;
  }

  svtkm::Vec<Vector3, 2> line(pointWCoords[idx - 1], pointWCoords[idx]);
  auto lpc = WorldCoordinatesToParametricCoordinates(
    line, wcoords, svtkm::CellShapeTagLine{}, success, worklet);

  //Segment param is [0,1] on that segment.
  //Map that onto the param for the entire segment.
  T dParam = static_cast<T>(1) / static_cast<T>(numPoints - 1);
  T polyLineParam = static_cast<T>(idx - 1) * dParam + lpc[0] * dParam;

  return Vector3(polyLineParam, 0, 0);
}

template <typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        svtkm::CellShapeTagPolygon,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& worklet)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, svtkm::CellShapeTagVertex{}, success, worklet);
    case 2:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, svtkm::CellShapeTagLine{}, success, worklet);
    default:
      return internal::WorldCoordinatesToParametricCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, wcoords, success, worklet);
  }
}

static inline SVTKM_EXEC svtkm::Vec3f WorldCoordinatesToParametricCoordinates(
  const svtkm::VecAxisAlignedPointCoordinates<2>& pointWCoords,
  const svtkm::Vec3f& wcoords,
  svtkm::CellShapeTagQuad,
  bool& success,
  const FunctorBase& worklet)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, wcoords, success, worklet);
}

static inline SVTKM_EXEC svtkm::Vec3f WorldCoordinatesToParametricCoordinates(
  const svtkm::VecAxisAlignedPointCoordinates<3>& pointWCoords,
  const svtkm::Vec3f& wcoords,
  svtkm::CellShapeTagHexahedron,
  bool& success,
  const FunctorBase& worklet)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, wcoords, success, worklet);
}

//-----------------------------------------------------------------------------
/// Returns the world paramteric corresponding to the given world coordinate for a cell.
///
template <typename WorldCoordVector>
static inline SVTKM_EXEC typename WorldCoordVector::ComponentType
WorldCoordinatesToParametricCoordinates(const WorldCoordVector& pointWCoords,
                                        const typename WorldCoordVector::ComponentType& wcoords,
                                        svtkm::CellShapeTagGeneric shape,
                                        bool& success,
                                        const svtkm::exec::FunctorBase& worklet)
{
  typename WorldCoordVector::ComponentType result;
  switch (shape.Id)
  {
    svtkmGenericCellShapeMacro(result = WorldCoordinatesToParametricCoordinates(
                                pointWCoords, wcoords, CellShapeTag(), success, worklet));
    default:
      success = false;
      worklet.RaiseError("Unknown cell shape sent to world 2 parametric.");
      return typename WorldCoordVector::ComponentType();
  }

  return result;
}
}
} // namespace svtkm::exec

#endif //svtk_m_exec_ParametricCoordinates_h
