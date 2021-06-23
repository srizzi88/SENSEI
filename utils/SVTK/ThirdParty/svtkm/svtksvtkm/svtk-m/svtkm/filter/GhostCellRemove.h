//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_GhostCellRemove_h
#define svtk_m_filter_GhostCellRemove_h

#include <svtkm/CellClassification.h>
#include <svtkm/filter/FilterDataSetWithField.h>
#include <svtkm/filter/Threshold.h>

namespace svtkm
{
namespace filter
{

struct GhostCellRemovePolicy : svtkm::filter::PolicyBase<GhostCellRemovePolicy>
{
  using FieldTypeList = svtkm::List<svtkm::UInt8>;
};

/// \brief Removes ghost cells
///
class GhostCellRemove : public svtkm::filter::FilterDataSetWithField<GhostCellRemove>
{
public:
  //currently the GhostCellRemove filter only works on uint8 data.
  using SupportedTypes = svtkm::List<svtkm::UInt8>;

  SVTKM_CONT
  GhostCellRemove();

  SVTKM_CONT
  void RemoveGhostField() { this->RemoveField = true; }
  SVTKM_CONT
  void RemoveAllGhost() { this->RemoveAll = true; }
  SVTKM_CONT
  void RemoveByType(const svtkm::UInt8& vals)
  {
    this->RemoveAll = false;
    this->RemoveVals = vals;
  }
  SVTKM_CONT
  bool GetRemoveGhostField() { return this->RemoveField; }
  SVTKM_CONT
  bool GetRemoveAllGhost() const { return this->RemoveAll; }

  SVTKM_CONT
  bool GetRemoveByType() const { return !this->RemoveAll; }
  SVTKM_CONT
  svtkm::UInt8 GetRemoveType() const { return this->RemoveVals; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          const svtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const svtkm::filter::FieldMetadata& fieldMeta,
                                          svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  bool RemoveAll;
  bool RemoveField;
  svtkm::UInt8 RemoveVals;
  svtkm::worklet::Threshold Worklet;
};
}
} // namespace svtkm::filter

#ifndef svtk_m_filter_GhostCellRemove_hxx
#include <svtkm/filter/GhostCellRemove.hxx>
#endif

#endif // svtk_m_filter_GhostCellRemove_h
