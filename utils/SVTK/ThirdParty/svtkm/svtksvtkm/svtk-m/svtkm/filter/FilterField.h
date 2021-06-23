//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_FieldFilter_h
#define svtk_m_filter_FieldFilter_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/PartitionedDataSet.h>

#include <svtkm/filter/Filter.h>
#include <svtkm/filter/PolicyBase.h>

namespace svtkm
{
namespace filter
{

template <class Derived>
class FilterField : public svtkm::filter::Filter<Derived>
{
public:
  SVTKM_CONT
  FilterField();

  SVTKM_CONT
  ~FilterField();

  SVTKM_CONT
  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  SVTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }

  //@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  SVTKM_CONT
  void SetActiveField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->ActiveFieldName = name;
    this->ActiveFieldAssociation = association;
  }

  SVTKM_CONT const std::string& GetActiveFieldName() const { return this->ActiveFieldName; }
  SVTKM_CONT svtkm::cont::Field::Association GetActiveFieldAssociation() const
  {
    return this->ActiveFieldAssociation;
  }
  //@}

  //@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  SVTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { this->UseCoordinateSystemAsField = val; }
  SVTKM_CONT
  bool GetUseCoordinateSystemAsField() const { return this->UseCoordinateSystemAsField; }
  //@}


  //@{
  /// Select the coordinate system index to make active to use when processing the input
  /// DataSet. This is used primarily by the Filter to select the coordinate system
  /// to use as a field when \c UseCoordinateSystemAsField is true.
  SVTKM_CONT
  void SetActiveCoordinateSystem(svtkm::Id index) { this->CoordinateSystemIndex = index; }

  SVTKM_CONT
  svtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }
  //@}

  //@{
  /// These are provided to satisfy the Filter API requirements.
  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet PrepareForExecution(const svtkm::cont::DataSet& input,
                                                    svtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet PrepareForExecution(const svtkm::cont::DataSet& input,
                                                    const svtkm::cont::Field& field,
                                                    svtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet PrepareForExecution(const svtkm::cont::DataSet& input,
                                                    const svtkm::cont::CoordinateSystem& field,
                                                    svtkm::filter::PolicyBase<DerivedPolicy> policy);
  //@}

protected:
private:
  std::string OutputFieldName;
  svtkm::Id CoordinateSystemIndex;
  std::string ActiveFieldName;
  svtkm::cont::Field::Association ActiveFieldAssociation;
  bool UseCoordinateSystemAsField;

  friend class svtkm::filter::Filter<Derived>;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/FilterField.hxx>

#endif // svtk_m_filter_FieldFilter_h
