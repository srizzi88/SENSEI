//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef svtk_m_worklet_CellSkewMetric_h
#define svtk_m_worklet_CellSkewMetric_h
/*
*/

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
#include "TypeOfCellTriangle.h"
#include "svtkm/CellShape.h"
#include "svtkm/CellTraits.h"
#include "svtkm/VecTraits.h"
#include "svtkm/VectorAnalysis.h"
#include "svtkm/exec/FunctorBase.h"
#include "svtkm/worklet/cellmetrics/CellConditionMetric.h"

namespace svtkm
{
namespace worklet
{
namespace cellmetrics
{
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellSkewMetric(const svtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 CellShapeType shape,
                                 const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  UNUSED(worklet);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellSkewMetric(const svtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 svtkm::CellShapeTagHexahedron,
                                 const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using Vector = typename PointCoordVecType::ComponentType;
  Vector X1 = (pts[1] - pts[0]) + (pts[2] - pts[3]) + (pts[5] - pts[4]) + (pts[6] - pts[7]);
  Scalar X1_mag = svtkm::Magnitude(X1);
  if (X1_mag <= Scalar(0.0))
    return svtkm::Infinity<Scalar>();
  Vector x1 = X1 / X1_mag;
  Vector X2 = (pts[3] - pts[0]) + (pts[2] - pts[1]) + (pts[7] - pts[4]) + (pts[6] - pts[5]);
  Scalar X2_mag = svtkm::Magnitude(X2);
  if (X2_mag <= Scalar(0.0))
    return svtkm::Infinity<Scalar>();
  Vector x2 = X2 / X2_mag;
  Vector X3 = (pts[4] - pts[0]) + (pts[5] - pts[1]) + (pts[6] - pts[2]) + (pts[7] - pts[3]);
  Scalar X3_mag = svtkm::Magnitude(X3);
  if (X3_mag <= Scalar(0.0))
    return svtkm::Infinity<Scalar>();
  Vector x3 = X3 / X3_mag;
  return svtkm::Max(svtkm::Dot(x1, x2), svtkm::Max(svtkm::Dot(x1, x3), svtkm::Dot(x2, x3)));
}

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellSkewMetric(const svtkm::IdComponent& numPts,
                                 const PointCoordVecType& pts,
                                 svtkm::CellShapeTagQuad,
                                 const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;
  const Vector X0 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X1 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar X0Mag = svtkm::Magnitude(X0);
  const Scalar X1Mag = svtkm::Magnitude(X1);

  if (X0Mag < Scalar(0.0) || X1Mag < Scalar(0.0))
    return Scalar(0.0);
  const Vector x0Normalized = X0 / X0Mag;
  const Vector x1Normalized = X1 / X1Mag;
  const Scalar dot = svtkm::Dot(x0Normalized, x1Normalized);
  return svtkm::Abs(dot);
}
}
} // worklet
} // svtkm
#endif
