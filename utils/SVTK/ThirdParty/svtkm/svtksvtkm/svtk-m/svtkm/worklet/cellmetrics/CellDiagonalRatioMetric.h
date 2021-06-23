//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtk_m_worklet_cellmetrics_CellDiagonalRatioMetric_h
#define svtk_m_worklet_cellmetrics_CellDiagonalRatioMetric_h

/*
* Mesh quality metric functions that compute the diagonal ratio of mesh cells.
* The diagonal ratio of a cell is defined as the length (magnitude) of the longest
* cell diagonal length divided by the length of the shortest cell diagonal length.
** These metric computations are adapted from the SVTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** The edge ratio computations for a pyramid cell types is not defined in the
* SVTK implementation, but is provided here.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: svtk/ThirdParty/verdict/svtkverdict (for SVTK code implementation of this metric)
*/

#include "svtkm/CellShape.h"
#include "svtkm/CellTraits.h"
#include "svtkm/VecTraits.h"
#include "svtkm/VectorAnalysis.h"
#include "svtkm/exec/FunctorBase.h"

#define UNUSED(expr) (void)(expr);

namespace svtkm
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = svtkm::FloatDefault;

template <typename OutType, typename VecType>
SVTKM_EXEC inline OutType ComputeDiagonalRatio(const VecType& diagonals)
{
  const svtkm::Id numDiagonals = diagonals.GetNumberOfComponents();

  //Compare diagonal lengths to determine the longest and shortest
  //TODO: Could we use lambda expression here?

  FloatType d0Len = (FloatType)svtkm::MagnitudeSquared(diagonals[0]);
  FloatType currLen, minLen = d0Len, maxLen = d0Len;
  for (int i = 1; i < numDiagonals; i++)
  {
    currLen = (FloatType)svtkm::MagnitudeSquared(diagonals[i]);
    if (currLen < minLen)
      minLen = currLen;
    if (currLen > maxLen)
      maxLen = currLen;
  }

  if (minLen <= OutType(0.0))
    return svtkm::Infinity<OutType>();

  //Take square root because we only did magnitude squared before
  OutType diagonalRatio = (OutType)svtkm::Sqrt(minLen / maxLen);
  return diagonalRatio;
}

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellDiagonalRatioMetric(const svtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          CellShapeType shape,
                                          const svtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.0);
}

// ========================= 2D cells ==================================
// Compute the diagonal ratio of a quadrilateral.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit square
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellDiagonalRatioMetric(const svtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          svtkm::CellShapeTagQuad,
                                          const svtkm::exec::FunctorBase& worklet)
{
  if (numPts != 4)
  {
    worklet.RaiseError("Diagonal ratio metric(quad) requires 4 points.");
    return OutType(0.0);
  }

  svtkm::IdComponent numDiagonals = 2; //pts.GetNumberOfComponents();

  //The 2 diagonals of a quadrilateral
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal QuadDiagonals[2] = { pts[2] - pts[0], pts[3] - pts[1] };

  return svtkm::worklet::cellmetrics::ComputeDiagonalRatio<OutType>(
    svtkm::make_VecC(QuadDiagonals, numDiagonals));
}

// ============================= 3D Volume cells ==================================
// Compute the diagonal ratio of a hexahedron.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit cube
// Acceptable Range: [0.65, 1]
// Normal Range: [0, 1]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellDiagonalRatioMetric(const svtkm::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          svtkm::CellShapeTagHexahedron,
                                          const svtkm::exec::FunctorBase& worklet)
{
  if (numPts != 8)
  {
    worklet.RaiseError("Diagonal ratio metric(hexahedron) requires 8 points.");
    return OutType(0.0);
  }

  svtkm::IdComponent numDiagonals = 4; //pts.GetNumberOfComponents();

  //The 4 diagonals of a hexahedron
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal HexDiagonals[4] = {
    pts[6] - pts[0], pts[7] - pts[1], pts[4] - pts[2], pts[5] - pts[3]
  };

  return svtkm::worklet::cellmetrics::ComputeDiagonalRatio<OutType>(
    svtkm::make_VecC(HexDiagonals, numDiagonals));
}
} // namespace cellmetrics
} // namespace worklet
} // namespace svtkm
#endif // svtk_m_worklet_cellmetrics_CellEdgeRatioMetric_h
