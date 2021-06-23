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
#ifndef svtk_m_worklet_cellmetrics_CellShapeAndSizeMetric_h
#define svtk_m_worklet_cellmetrics_CellShapeAndSizeMetric_h
/*
 * Mesh quality metric functions that compute the shape and size of a cell. This
 * takes the shape metric and multiplies it by the relative size squared metric.
 *
 * These metric computations are adapted from the SVTK implementation of the
 * Verdict library, which provides a set of mesh/cell metrics for evaluating the
 * geometric qualities of regions of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: svtk/ThirdParty/verdict/svtkverdict (for SVTK code implementation of this
 * metric)
 */

#include "svtkm/CellShape.h"
#include "svtkm/CellTraits.h"
#include "svtkm/VecTraits.h"
#include "svtkm/VectorAnalysis.h"
#include "svtkm/exec/FunctorBase.h"
#include "svtkm/worklet/cellmetrics/CellShapeMetric.h"

#define UNUSED(expr) (void)(expr);

namespace svtkm
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = svtkm::FloatDefault;

// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
SVTKM_EXEC OutType CellShapeAndSizeMetric(const svtkm::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         const OutType& avgArea,
                                         CellShapeType shape,
                                         const svtkm::exec::FunctorBase&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(avgArea);
  UNUSED(shape);
  return OutType(-1.);
}

// ========================= 2D cells ==================================

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellShapeAndSizeMetric(const svtkm::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         const OutType& avgArea,
                                         svtkm::CellShapeTagTriangle tag,
                                         const svtkm::exec::FunctorBase& worklet)
{
  OutType rss = svtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgArea, tag, worklet);
  OutType shape = svtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, worklet);
  OutType q = rss * shape;
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellShapeAndSizeMetric(const svtkm::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         const OutType& avgArea,
                                         svtkm::CellShapeTagQuad tag,
                                         const svtkm::exec::FunctorBase& worklet)
{
  OutType rss = svtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgArea, tag, worklet);
  OutType shape = svtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, worklet);
  OutType q = rss * shape;
  return OutType(q);
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellShapeAndSizeMetric(const svtkm::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         const OutType& avgVolume,
                                         svtkm::CellShapeTagTetra tag,
                                         const svtkm::exec::FunctorBase& worklet)
{
  OutType rss = svtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgVolume, tag, worklet);
  OutType shape = svtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, worklet);
  OutType q = rss * shape;
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
SVTKM_EXEC OutType CellShapeAndSizeMetric(const svtkm::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         const OutType& avgVolume,
                                         svtkm::CellShapeTagHexahedron tag,
                                         const svtkm::exec::FunctorBase& worklet)
{
  OutType rss = svtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgVolume, tag, worklet);
  OutType shape = svtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, worklet);
  OutType q = rss * shape;
  return OutType(q);
}

} // namespace cellmetrics
} // namespace worklet
} // namespace svtkm

#endif // svtk_m_exec_cellmetrics_CellShapeAndSizeMetric.h
