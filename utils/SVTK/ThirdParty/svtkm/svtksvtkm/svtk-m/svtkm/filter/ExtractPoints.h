//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_ExtractPoints_h
#define svtk_m_filter_ExtractPoints_h

#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/ExtractPoints.h>

namespace svtkm
{
namespace filter
{
/// @brief Extract only points from a geometry using an implicit function
///
///
/// Extract only the  points that are either inside or outside of a
/// SVTK-m implicit function. Examples include planes, spheres, boxes,
/// etc.
/// Note that while any geometry type can be provided as input, the output is
/// represented by an explicit representation of points using
/// svtkm::cont::CellSetSingleType
class ExtractPoints : public svtkm::filter::FilterDataSet<ExtractPoints>
{
public:
  SVTKM_CONT
  ExtractPoints();

  /// When CompactPoints is set, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the unused elements
  SVTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  SVTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// Set the volume of interest to extract
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

  template <typename DerivedPolicy>
  svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(svtkm::cont::DataSet& result,
                  const svtkm::cont::ArrayHandle<T, StorageType>& input,
                  const svtkm::filter::FieldMetadata& fieldMeta,
                  svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  bool ExtractInside;
  svtkm::cont::ImplicitFunctionHandle Function;

  bool CompactPoints;
  svtkm::filter::CleanGrid Compactor;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/ExtractPoints.hxx>

#endif // svtk_m_filter_ExtractPoints_h
