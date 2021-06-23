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
//=========================================================================
#ifndef svtk_m_filter_MeshQuality_hxx
#define svtk_m_filter_MeshQuality_hxx

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/Field.h>
#include <svtkm/filter/CellMeasures.h>
#include <svtkm/filter/CreateResult.h>

// #define DEBUG_PRINT

namespace svtkm
{
namespace filter
{


inline SVTKM_CONT MeshQuality::MeshQuality(CellMetric metric)
  : svtkm::filter::FilterField<MeshQuality>()
{
  this->SetUseCoordinateSystemAsField(true);
  this->MyMetric = metric;
  if (this->MyMetric < CellMetric::AREA || this->MyMetric >= CellMetric::NUMBER_OF_CELL_METRICS)
  {
    SVTKM_ASSERT(true);
  }
  this->SetOutputFieldName(MetricNames[(int)this->MyMetric]);
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet MeshQuality::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& points,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  SVTKM_ASSERT(fieldMeta.IsPointField());

  //TODO: Should other cellset types be supported?
  svtkm::cont::CellSetExplicit<> cellSet;
  input.GetCellSet().CopyTo(cellSet);

  svtkm::worklet::MeshQuality<CellMetric> qualityWorklet;

  if (this->MyMetric == svtkm::filter::CellMetric::RELATIVE_SIZE_SQUARED ||
      this->MyMetric == svtkm::filter::CellMetric::SHAPE_AND_SIZE)
  {
    svtkm::FloatDefault averageArea = 1.;
    svtkm::worklet::MeshQuality<CellMetric> subWorklet;
    svtkm::cont::ArrayHandle<T> array;
    subWorklet.SetMetric(svtkm::filter::CellMetric::AREA);
    this->Invoke(subWorklet, svtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, array);
    T zero = 0.0;
    svtkm::FloatDefault totalArea = (svtkm::FloatDefault)svtkm::cont::Algorithm::Reduce(array, zero);

    svtkm::FloatDefault averageVolume = 1.;
    subWorklet.SetMetric(svtkm::filter::CellMetric::VOLUME);
    this->Invoke(subWorklet, svtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, array);
    svtkm::FloatDefault totalVolume = (svtkm::FloatDefault)svtkm::cont::Algorithm::Reduce(array, zero);

    svtkm::Id numVals = array.GetNumberOfValues();
    if (numVals > 0)
    {
      averageArea = totalArea / static_cast<svtkm::FloatDefault>(numVals);
      averageVolume = totalVolume / static_cast<svtkm::FloatDefault>(numVals);
    }
    qualityWorklet.SetAverageArea(averageArea);
    qualityWorklet.SetAverageVolume(averageVolume);
  }

  //Invoke the MeshQuality worklet
  svtkm::cont::ArrayHandle<T> outArray;
  qualityWorklet.SetMetric(this->MyMetric);
  this->Invoke(qualityWorklet, svtkm::filter::ApplyPolicyCellSet(cellSet, policy), points, outArray);

  svtkm::cont::DataSet result;
  result.CopyStructure(input); //clone of the input dataset

  //Append the metric values of all cells into the output
  //dataset as a new field
  result.AddField(svtkm::cont::make_FieldCell(this->GetOutputFieldName(), outArray));

  return result;
}

} // namespace filter
} // namespace svtkm
#endif
