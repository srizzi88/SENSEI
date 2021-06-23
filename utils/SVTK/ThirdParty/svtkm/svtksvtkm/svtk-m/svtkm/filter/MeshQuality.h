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

#ifndef svtk_m_filter_MeshQuality_h
#define svtk_m_filter_MeshQuality_h

#include <svtkm/CellShape.h>
#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/FieldStatistics.h>
#include <svtkm/worklet/MeshQuality.h>

namespace svtkm
{
namespace filter
{

//Names of the available cell metrics, for use in
//the output dataset fields
static const std::string MetricNames[] = { "area",
                                           "aspectGamma",
                                           "aspectRatio",
                                           "condition",
                                           "diagonalRatio",
                                           "dimension",
                                           "jacobian",
                                           "maxAngle",
                                           "maxDiagonal",
                                           "minAngle",
                                           "minDiagonal",
                                           "oddy",
                                           "relativeSizeSquared",
                                           "scaledJacobian",
                                           "shape",
                                           "shapeAndSize",
                                           "shear",
                                           "skew",
                                           "stretch",
                                           "taper",
                                           "volume",
                                           "warpage" };

//Different cell metrics available to use
//This must follow the same order as the MetricNames above
enum class CellMetric
{
  AREA,
  ASPECT_GAMMA,
  ASPECT_RATIO,
  CONDITION,
  DIAGONAL_RATIO,
  DIMENSION,
  JACOBIAN,
  MAX_ANGLE,
  MAX_DIAGONAL,
  MIN_ANGLE,
  MIN_DIAGONAL,
  ODDY,
  RELATIVE_SIZE_SQUARED,
  SCALED_JACOBIAN,
  SHAPE,
  SHAPE_AND_SIZE,
  SHEAR,
  SKEW,
  STRETCH,
  TAPER,
  VOLUME,
  WARPAGE,
  NUMBER_OF_CELL_METRICS,
  EMPTY
};

/** \brief Computes the quality of an unstructured cell-based mesh. The quality is defined in terms of the
  * summary statistics (frequency, mean, variance, min, max) of metrics computed over the mesh
  * cells. One of several different metrics can be specified for a given cell type, and the mesh
  * can consist of one or more different cell types. The resulting mesh quality is stored as one
  * or more new fields in the output dataset of this filter, with a separate field for each cell type.
  * Each field contains the metric summary statistics for the cell type.
  * Summary statists with all 0 values imply that the specified metric does not support the cell type.
  */
class MeshQuality : public svtkm::filter::FilterField<MeshQuality>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT MeshQuality(CellMetric);

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& points,
    const svtkm::filter::FieldMetadata& fieldMeta,
    const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  CellMetric MyMetric;
};

} // namespace filter
} // namespace svtkm

#include <svtkm/filter/MeshQuality.hxx>

#endif // svtk_m_filter_MeshQuality_h
