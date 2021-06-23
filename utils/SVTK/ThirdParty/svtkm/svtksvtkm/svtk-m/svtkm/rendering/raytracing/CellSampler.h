//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_CellSampler_h
#define svtk_m_rendering_raytracing_CellSampler_h

#include <svtkm/VecVariable.h>
#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/ParametricCoordinates.h>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{
template <typename CellTag>
SVTKM_EXEC_CONT inline svtkm::Int32 GetNumberOfPoints(CellTag tag);

template <>
SVTKM_EXEC_CONT inline svtkm::Int32 GetNumberOfPoints<svtkm::CellShapeTagHexahedron>(
  svtkm::CellShapeTagHexahedron svtkmNotUsed(tag))
{
  return 8;
}

template <>
SVTKM_EXEC_CONT inline svtkm::Int32 GetNumberOfPoints<svtkm::CellShapeTagTetra>(
  svtkm::CellShapeTagTetra svtkmNotUsed(tag))
{
  return 4;
}

template <>
SVTKM_EXEC_CONT inline svtkm::Int32 GetNumberOfPoints<svtkm::CellShapeTagWedge>(
  svtkm::CellShapeTagWedge svtkmNotUsed(tag))
{
  return 6;
}

template <>
SVTKM_EXEC_CONT inline svtkm::Int32 GetNumberOfPoints<svtkm::CellShapeTagPyramid>(
  svtkm::CellShapeTagPyramid svtkmNotUsed(tag))
{
  return 5;
}

template <typename P, typename S, typename WorkletType, typename CellShapeTagType>
SVTKM_EXEC_CONT inline bool Sample(const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
                                  const svtkm::Vec<S, 8>& scalars,
                                  const svtkm::Vec<P, 3>& sampleLocation,
                                  S& lerpedScalar,
                                  const WorkletType& callingWorklet,
                                  const CellShapeTagType& shapeTag)
{

  bool validSample = true;
  svtkm::VecVariable<svtkm::Vec<P, 3>, 8> pointsVec;
  svtkm::VecVariable<S, 8> scalarVec;
  for (svtkm::Int32 i = 0; i < GetNumberOfPoints(shapeTag); ++i)
  {
    pointsVec.Append(points[i]);
    scalarVec.Append(scalars[i]);
  }
  bool success = false; // ignored
  svtkm::Vec<P, 3> pcoords = svtkm::exec::WorldCoordinatesToParametricCoordinates(
    pointsVec, sampleLocation, shapeTag, success, callingWorklet);
  P pmin, pmax;
  pmin = svtkm::Min(svtkm::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = svtkm::Max(svtkm::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  lerpedScalar = svtkm::exec::CellInterpolate(scalarVec, pcoords, shapeTag, callingWorklet);
  return validSample;
}

template <typename S, typename P, typename WorkletType, typename CellShapeTagType>
SVTKM_EXEC_CONT inline bool Sample(const svtkm::VecAxisAlignedPointCoordinates<3>& points,
                                  const svtkm::Vec<S, 8>& scalars,
                                  const svtkm::Vec<P, 3>& sampleLocation,
                                  S& lerpedScalar,
                                  const WorkletType& callingWorklet,
                                  const CellShapeTagType& svtkmNotUsed(shapeTag))
{

  bool validSample = true;
  bool success;
  svtkm::Vec<P, 3> pcoords = svtkm::exec::WorldCoordinatesToParametricCoordinates(
    points, sampleLocation, svtkm::CellShapeTagHexahedron(), success, callingWorklet);
  P pmin, pmax;
  pmin = svtkm::Min(svtkm::Min(pcoords[0], pcoords[1]), pcoords[2]);
  pmax = svtkm::Max(svtkm::Max(pcoords[0], pcoords[1]), pcoords[2]);
  if (pmin < 0.f || pmax > 1.f)
  {
    validSample = false;
  }
  lerpedScalar =
    svtkm::exec::CellInterpolate(scalars, pcoords, svtkm::CellShapeTagHexahedron(), callingWorklet);
  return validSample;
}
} // namespace detail

//
//  General Template: returns false if sample location is outside the cell
//
template <int CellType>
class CellSampler
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(const svtkm::Vec<svtkm::Vec<P, 3>, 8>& svtkmNotUsed(points),
                                        const svtkm::Vec<S, 8>& svtkmNotUsed(scalars),
                                        const svtkm::Vec<P, 3>& svtkmNotUsed(sampleLocation),
                                        S& svtkmNotUsed(lerpedScalar),
                                        const WorkletType& svtkmNotUsed(callingWorklet),
                                        const svtkm::Int32& svtkmNotUsed(cellShape = CellType)) const
  {
    static_assert(CellType != CELL_SHAPE_ZOO && CellType != CELL_SHAPE_STRUCTURED &&
                    CellType != CELL_SHAPE_HEXAHEDRON && CellType != CELL_SHAPE_TETRA &&
                    CellType != CELL_SHAPE_WEDGE && CellType != CELL_SHAPE_PYRAMID,
                  "Cell Sampler: Default template. This should not happen.\n");
    return false;
  }
};

//
// Zoo Sampler
//
template <>
class CellSampler<255>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
                                        const svtkm::Vec<S, 8>& scalars,
                                        const svtkm::Vec<P, 3>& sampleLocation,
                                        S& lerpedScalar,
                                        const WorkletType& callingWorklet,
                                        const svtkm::Int32& cellShape) const
  {
    bool valid = false;
    if (cellShape == CELL_SHAPE_HEXAHEDRON)
    {
      valid = detail::Sample(points,
                             scalars,
                             sampleLocation,
                             lerpedScalar,
                             callingWorklet,
                             svtkm::CellShapeTagHexahedron());
    }

    if (cellShape == CELL_SHAPE_TETRA)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagTetra());
    }

    if (cellShape == CELL_SHAPE_WEDGE)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagWedge());
    }
    if (cellShape == CELL_SHAPE_PYRAMID)
    {
      valid = detail::Sample(
        points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagPyramid());
    }
    return valid;
  }
};

//
//  Single type hex
//
template <>
class CellSampler<CELL_SHAPE_HEXAHEDRON>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(
    const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
    const svtkm::Vec<S, 8>& scalars,
    const svtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const svtkm::Int32& svtkmNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    return detail::Sample(points,
                          scalars,
                          sampleLocation,
                          lerpedScalar,
                          callingWorklet,
                          svtkm::CellShapeTagHexahedron());
  }
};

//
//  Single type hex uniform and rectilinear
//  calls fast path for sampling
//
template <>
class CellSampler<CELL_SHAPE_STRUCTURED>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(
    const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
    const svtkm::Vec<S, 8>& scalars,
    const svtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const svtkm::Int32& svtkmNotUsed(cellShape = CELL_SHAPE_HEXAHEDRON)) const
  {
    svtkm::VecAxisAlignedPointCoordinates<3> rPoints(points[0], points[6] - points[0]);
    return detail::Sample(rPoints,
                          scalars,
                          sampleLocation,
                          lerpedScalar,
                          callingWorklet,
                          svtkm::CellShapeTagHexahedron());
  }
};

//
//  Single type pyramid
//
template <>
class CellSampler<CELL_SHAPE_PYRAMID>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(
    const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
    const svtkm::Vec<S, 8>& scalars,
    const svtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const svtkm::Int32& svtkmNotUsed(cellShape = CELL_SHAPE_PYRAMID)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagPyramid());
  }
};


//
//  Single type Tet
//
template <>
class CellSampler<CELL_SHAPE_TETRA>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(
    const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
    const svtkm::Vec<S, 8>& scalars,
    const svtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const svtkm::Int32& svtkmNotUsed(cellShape = CELL_SHAPE_TETRA)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagTetra());
  }
};

//
//  Single type Wedge
//
template <>
class CellSampler<CELL_SHAPE_WEDGE>
{
public:
  template <typename P, typename S, typename WorkletType>
  SVTKM_EXEC_CONT inline bool SampleCell(
    const svtkm::Vec<svtkm::Vec<P, 3>, 8>& points,
    const svtkm::Vec<S, 8>& scalars,
    const svtkm::Vec<P, 3>& sampleLocation,
    S& lerpedScalar,
    const WorkletType& callingWorklet,
    const svtkm::Int32& svtkmNotUsed(cellShape = CELL_SHAPE_WEDGE)) const
  {
    return detail::Sample(
      points, scalars, sampleLocation, lerpedScalar, callingWorklet, svtkm::CellShapeTagWedge());
  }
};
}
}
} // namespace svtkm::rendering::raytracing
#endif
