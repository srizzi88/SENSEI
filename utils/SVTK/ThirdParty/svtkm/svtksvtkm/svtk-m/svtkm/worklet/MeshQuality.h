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

#ifndef svtk_m_worklet_MeshQuality_h
#define svtk_m_worklet_MeshQuality_h

#include "svtkm/worklet/CellMeasure.h"
#include "svtkm/worklet/WorkletMapTopology.h"
#include "svtkm/worklet/cellmetrics/CellAspectGammaMetric.h"
#include "svtkm/worklet/cellmetrics/CellAspectRatioMetric.h"
#include "svtkm/worklet/cellmetrics/CellConditionMetric.h"
#include "svtkm/worklet/cellmetrics/CellDiagonalRatioMetric.h"
#include "svtkm/worklet/cellmetrics/CellDimensionMetric.h"
#include "svtkm/worklet/cellmetrics/CellJacobianMetric.h"
#include "svtkm/worklet/cellmetrics/CellMaxAngleMetric.h"
#include "svtkm/worklet/cellmetrics/CellMaxDiagonalMetric.h"
#include "svtkm/worklet/cellmetrics/CellMinAngleMetric.h"
#include "svtkm/worklet/cellmetrics/CellMinDiagonalMetric.h"
#include "svtkm/worklet/cellmetrics/CellOddyMetric.h"
#include "svtkm/worklet/cellmetrics/CellRelativeSizeSquaredMetric.h"
#include "svtkm/worklet/cellmetrics/CellScaledJacobianMetric.h"
#include "svtkm/worklet/cellmetrics/CellShapeAndSizeMetric.h"
#include "svtkm/worklet/cellmetrics/CellShapeMetric.h"
#include "svtkm/worklet/cellmetrics/CellShearMetric.h"
#include "svtkm/worklet/cellmetrics/CellSkewMetric.h"
#include "svtkm/worklet/cellmetrics/CellStretchMetric.h"
#include "svtkm/worklet/cellmetrics/CellTaperMetric.h"
#include "svtkm/worklet/cellmetrics/CellWarpageMetric.h"

namespace svtkm
{
namespace worklet
{

/**
  * Worklet that computes mesh quality metric values for each cell in
  * the input mesh. A metric is specified per cell type in the calling filter,
  * and this metric is invoked over all cells of that cell type. An array of
  * the computed metric values (one per cell) is returned as output.
  */
template <typename MetricTagType>
class MeshQuality : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  void SetMetric(MetricTagType m) { this->Metric = m; }
  void SetAverageArea(svtkm::FloatDefault a) { this->AverageArea = a; };
  void SetAverageVolume(svtkm::FloatDefault v) { this->AverageVolume = v; };

  template <typename CellShapeType, typename PointCoordVecType, typename OutType>
  SVTKM_EXEC void operator()(CellShapeType shape,
                            const svtkm::IdComponent& numPoints,
                            //const CountsArrayType& counts,
                            //const MetricsArrayType& metrics,
                            //MetricTagType metric,
                            const PointCoordVecType& pts,
                            OutType& metricValue) const
  {
    svtkm::UInt8 thisId = shape.Id;
    if (shape.Id == svtkm::CELL_SHAPE_POLYGON)
    {
      if (numPoints == 3)
        thisId = svtkm::CELL_SHAPE_TRIANGLE;
      else if (numPoints == 4)
        thisId = svtkm::CELL_SHAPE_QUAD;
    }
    switch (thisId)
    {
      svtkmGenericCellShapeMacro(metricValue =
                                  this->ComputeMetric<OutType>(numPoints, pts, CellShapeTag()));
      default:
        this->RaiseError("Asked for metric of unknown cell type.");
        metricValue = OutType(0.0);
    }
  }

protected:
  // data member
  MetricTagType Metric;
  svtkm::FloatDefault AverageArea;
  svtkm::FloatDefault AverageVolume;

  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  SVTKM_EXEC OutType ComputeMetric(const svtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType tag) const
  {
    constexpr svtkm::IdComponent dims = svtkm::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS;

    //Only compute the metric for 2D and 3D shapes; return 0 otherwise
    OutType metricValue = OutType(0.0);
    svtkm::FloatDefault average = (dims == 2 ? this->AverageArea : this->AverageVolume);

    if (dims > 0)
    {
      switch (this->Metric)
      {
        case MetricTagType::AREA:
          metricValue = svtkm::exec::CellMeasure<OutType>(numPts, pts, tag, *this);
          if (dims != 2)
            metricValue = 0.;
          break;
        case MetricTagType::ASPECT_GAMMA:
          metricValue =
            svtkm::worklet::cellmetrics::CellAspectGammaMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::ASPECT_RATIO:
          metricValue =
            svtkm::worklet::cellmetrics::CellAspectRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::CONDITION:
          metricValue =
            svtkm::worklet::cellmetrics::CellConditionMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::DIAGONAL_RATIO:
          metricValue =
            svtkm::worklet::cellmetrics::CellDiagonalRatioMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::DIMENSION:
          metricValue =
            svtkm::worklet::cellmetrics::CellDimensionMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::JACOBIAN:
          metricValue =
            svtkm::worklet::cellmetrics::CellJacobianMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MAX_ANGLE:
          metricValue =
            svtkm::worklet::cellmetrics::CellMaxAngleMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MAX_DIAGONAL:
          metricValue =
            svtkm::worklet::cellmetrics::CellMaxDiagonalMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MIN_ANGLE:
          metricValue =
            svtkm::worklet::cellmetrics::CellMinAngleMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::MIN_DIAGONAL:
          metricValue =
            svtkm::worklet::cellmetrics::CellMinDiagonalMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::ODDY:
          metricValue =
            svtkm::worklet::cellmetrics::CellOddyMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::RELATIVE_SIZE_SQUARED:
          metricValue = svtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, *this);
          break;
        case MetricTagType::SHAPE_AND_SIZE:
          metricValue = svtkm::worklet::cellmetrics::CellShapeAndSizeMetric<OutType>(
            numPts, pts, static_cast<OutType>(average), tag, *this);
          break;
        case MetricTagType::SCALED_JACOBIAN:
          metricValue =
            svtkm::worklet::cellmetrics::CellScaledJacobianMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SHAPE:
          metricValue =
            svtkm::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SHEAR:
          metricValue =
            svtkm::worklet::cellmetrics::CellShearMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::SKEW:
          metricValue =
            svtkm::worklet::cellmetrics::CellSkewMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::STRETCH:
          metricValue =
            svtkm::worklet::cellmetrics::CellStretchMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::TAPER:
          metricValue =
            svtkm::worklet::cellmetrics::CellTaperMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::VOLUME:
          metricValue = svtkm::exec::CellMeasure<OutType>(numPts, pts, tag, *this);
          if (dims != 3)
            metricValue = 0.;
          break;
        case MetricTagType::WARPAGE:
          metricValue =
            svtkm::worklet::cellmetrics::CellWarpageMetric<OutType>(numPts, pts, tag, *this);
          break;
        case MetricTagType::EMPTY:
          break;
        default:
          //Only call metric function if a metric is specified for this shape type
          this->RaiseError("Asked for unknown metric.");
      }
    }

    return metricValue;
  }
};

} // namespace worklet
} // namespace svtkm

#endif // svtk_m_worklet_MeshQuality_h
