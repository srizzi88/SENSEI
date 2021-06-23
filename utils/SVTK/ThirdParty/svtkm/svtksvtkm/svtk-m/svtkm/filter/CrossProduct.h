//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_CrossProduct_h
#define svtk_m_filter_CrossProduct_h

#include <svtkm/filter/FilterField.h>
#include <svtkm/worklet/CrossProduct.h>

namespace svtkm
{
namespace filter
{

class CrossProduct : public svtkm::filter::FilterField<CrossProduct>
{
public:
  //currently the DotProduct filter only works on vector data.
  using SupportedTypes = TypeListVecCommon;

  SVTKM_CONT
  CrossProduct();

  //@{
  /// Choose the primary field to operate on. In the cross product operation A x B, A is
  /// the primary field.
  SVTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->SetActiveField(name, association);
  }

  SVTKM_CONT const std::string& GetPrimaryFieldName() const { return this->GetActiveFieldName(); }
  SVTKM_CONT svtkm::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  SVTKM_CONT
  void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  SVTKM_CONT
  bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  SVTKM_CONT
  void SetPrimaryCoordinateSystem(svtkm::Id index) { this->SetActiveCoordinateSystem(index); }
  SVTKM_CONT
  svtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }
  //@}

  //@{
  /// Choose the secondary field to operate on. In the cross product operation A x B, B is
  /// the secondary field.
  SVTKM_CONT
  void SetSecondaryField(
    const std::string& name,
    svtkm::cont::Field::Association association = svtkm::cont::Field::Association::ANY)
  {
    this->SecondaryFieldName = name;
    this->SecondaryFieldAssociation = association;
  }

  SVTKM_CONT const std::string& GetSecondaryFieldName() const { return this->SecondaryFieldName; }
  SVTKM_CONT svtkm::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->SecondaryFieldAssociation;
  }
  //@}

  //@{
  /// When set to true, uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  SVTKM_CONT
  void SetUseCoordinateSystemAsSecondaryField(bool flag)
  {
    this->UseCoordinateSystemAsSecondaryField = flag;
  }
  SVTKM_CONT
  bool GetUseCoordinateSystemAsSecondaryField() const
  {
    return this->UseCoordinateSystemAsSecondaryField;
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  SVTKM_CONT
  void SetSecondaryCoordinateSystem(svtkm::Id index)
  {
    this->SecondaryCoordinateSystemIndex = index;
  }
  SVTKM_CONT
  svtkm::Id GetSecondaryCoordinateSystemIndex() const
  {
    return this->SecondaryCoordinateSystemIndex;
  }
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  std::string SecondaryFieldName;
  svtkm::cont::Field::Association SecondaryFieldAssociation;
  bool UseCoordinateSystemAsSecondaryField;
  svtkm::Id SecondaryCoordinateSystemIndex;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/CrossProduct.hxx>

#endif // svtk_m_filter_CrossProduct_h
