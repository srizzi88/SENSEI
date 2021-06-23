//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CellMeasures_h
#define svtk_m_filter_CellMeasures_h

#include <svtkm/filter/FilterCell.h>
#include <svtkm/worklet/CellMeasure.h>

namespace svtkm
{
namespace filter
{

/// \brief Compute the measure of each (3D) cell in a dataset.
///
/// CellMeasures is a filter that generates a new cell data array (i.e., one value
/// specified per cell) holding the signed measure of the cell
/// or 0 (if measure is not well defined or the cell type is unsupported).
///
/// By default, the new cell-data array is named "measure".
template <typename IntegrationType>
class CellMeasures : public svtkm::filter::FilterCell<CellMeasures<IntegrationType>>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  CellMeasures();

  /// Set/Get the name of the cell measure field. If empty, "measure" is used.
  void SetCellMeasureName(const std::string& name) { this->SetOutputFieldName(name); }
  const std::string& GetCellMeasureName() const { return this->GetOutputFieldName(); }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& points,
    const svtkm::filter::FieldMetadata& fieldMeta,
    const svtkm::filter::PolicyBase<DerivedPolicy>& policy);
};
}
} // namespace svtkm::filter

#include <svtkm/filter/CellMeasures.hxx>

#endif // svtk_m_filter_CellMeasures_h
