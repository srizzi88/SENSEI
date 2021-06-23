//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractGeometry_h
#define svtk_m_filter_ExtractGeometry_h

#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/ExtractGeometry.h>

namespace svtkm
{
namespace filter
{

/// \brief  Extract a subset of geometry based on an implicit function
///
/// Extracts from its input geometry all cells that are either
/// completely inside or outside of a specified implicit function. Any type of
/// data can be input to this filter.
///
/// To use this filter you must specify an implicit function. You must also
/// specify whether to extract cells laying inside or outside of the implicit
/// function. (The inside of an implicit function is the negative values
/// region.) An option exists to extract cells that are neither inside or
/// outside (i.e., boundary).
///
/// This differs from Clip in that Clip will subdivide boundary cells into new
/// cells, while this filter will not, producing a more 'crinkly' output.
///
class ExtractGeometry : public svtkm::filter::FilterDataSet<ExtractGeometry>
{
public:
  //currently the ExtractGeometry filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  SVTKM_CONT
  ExtractGeometry();

  // Set the volume of interest to extract
  void SetImplicitFunction(const svtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  const svtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  SVTKM_CONT
  bool GetExtractInside() { return this->ExtractInside; }
  SVTKM_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  SVTKM_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  SVTKM_CONT
  void ExtractInsideOff() { this->ExtractInside = false; }

  SVTKM_CONT
  bool GetExtractBoundaryCells() { return this->ExtractBoundaryCells; }
  SVTKM_CONT
  void SetExtractBoundaryCells(bool value) { this->ExtractBoundaryCells = value; }
  SVTKM_CONT
  void ExtractBoundaryCellsOn() { this->ExtractBoundaryCells = true; }
  SVTKM_CONT
  void ExtractBoundaryCellsOff() { this->ExtractBoundaryCells = false; }

  SVTKM_CONT
  bool GetExtractOnlyBoundaryCells() { return this->ExtractOnlyBoundaryCells; }
  SVTKM_CONT
  void SetExtractOnlyBoundaryCells(bool value) { this->ExtractOnlyBoundaryCells = value; }
  SVTKM_CONT
  void ExtractOnlyBoundaryCellsOn() { this->ExtractOnlyBoundaryCells = true; }
  SVTKM_CONT
  void ExtractOnlyBoundaryCellsOff() { this->ExtractOnlyBoundaryCells = false; }

  template <typename DerivedPolicy>
  svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(svtkm::cont::DataSet& result,
                  const svtkm::cont::ArrayHandle<T, StorageType>& input,
                  const svtkm::filter::FieldMetadata& fieldMeta,
                  const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;
  svtkm::cont::ImplicitFunctionHandle Function;
  svtkm::worklet::ExtractGeometry Worklet;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ExtractGeometry.hxx>

#endif // svtk_m_filter_ExtractGeometry_h
