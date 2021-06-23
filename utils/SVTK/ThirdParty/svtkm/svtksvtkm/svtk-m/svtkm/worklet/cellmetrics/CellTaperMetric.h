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
#ifndef svtk_m_worklet_CellTaperMetric_h
#define svtk_m_worklet_CellTaperMetric_h
/*
* Mesh quality metric functions that compute the shape, or weighted Jacobian, of mesh cells.
* The Jacobian of a cell is weighted by the condition metric value of the cell.
** These metric computations are adapted from the SVTK implementation of the Verdict library,
* which provides a set of cell metrics for evaluating the geometric qualities of regions of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: svtk/ThirdParty/verdict/svtkverdict (for SVTK code implementation of this metric)
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

namespace svtkm
{
namespace worklet
{
namespace cellmetrics
{
// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellTaperMetric(const svtkm::IdComponent& numPts,
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
// ========================= 2D cells ==================================
template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellTaperMetric(const svtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  svtkm::CellShapeTagQuad,
                                  const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector X12 = Vector((pts[0] - pts[1]) + (pts[2] - pts[3]));
  const Vector X1 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X2 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar x12 = svtkm::Sqrt(svtkm::MagnitudeSquared(X12));
  const Scalar x1 = svtkm::Sqrt(svtkm::MagnitudeSquared(X1));
  const Scalar x2 = svtkm::Sqrt(svtkm::MagnitudeSquared(X2));
  const Scalar minLength = svtkm::Min(x1, x2);

  if (minLength <= Scalar(0.0))
  {
    return svtkm::Infinity<Scalar>();
  }

  const Scalar q = x12 / minLength;
  return q;
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellTaperMetric(const svtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  svtkm::CellShapeTagHexahedron,
                                  const svtkm::exec::FunctorBase& worklet)
{
  UNUSED(numPts);
  UNUSED(worklet);
  using Scalar = OutType;

  Scalar X1 = svtkm::Sqrt(svtkm::MagnitudeSquared((pts[1] - pts[0]) + (pts[2] - pts[3]) +
                                                (pts[5] - pts[4]) + (pts[6] - pts[7])));
  Scalar X2 = svtkm::Sqrt(svtkm::MagnitudeSquared((pts[3] - pts[0]) + (pts[2] - pts[1]) +
                                                (pts[7] - pts[4]) + (pts[6] - pts[5])));
  Scalar X3 = svtkm::Sqrt(svtkm::MagnitudeSquared((pts[4] - pts[0]) + (pts[5] - pts[1]) +
                                                (pts[6] - pts[2]) + (pts[7] - pts[3])));
  if ((X1 <= Scalar(0.0)) || (X2 <= Scalar(0.0)) || (X3 <= Scalar(0.0)))
  {
    return svtkm::Infinity<Scalar>();
  }
  Scalar X12 = svtkm::Sqrt(svtkm::MagnitudeSquared(((pts[2] - pts[3]) - (pts[1] - pts[0])) +
                                                 ((pts[6] - pts[7]) - (pts[5] - pts[4]))));
  Scalar X13 = svtkm::Sqrt(svtkm::MagnitudeSquared(((pts[5] - pts[1]) - (pts[4] - pts[0])) +
                                                 ((pts[6] - pts[2]) - (pts[7] - pts[3]))));
  Scalar X23 = svtkm::Sqrt(svtkm::MagnitudeSquared(((pts[7] - pts[4]) - (pts[3] - pts[0])) +
                                                 ((pts[6] - pts[5]) - (pts[2] - pts[1]))));
  Scalar T12 = X12 / svtkm::Min(X1, X2);
  Scalar T13 = X13 / svtkm::Min(X1, X3);
  Scalar T23 = X23 / svtkm::Min(X2, X3);
  return svtkm::Max(T12, svtkm::Max(T13, T23));
}
}
} // worklet
} // svtkm
#endif // svtk_m_worklet_CellTaper_Metric_h
