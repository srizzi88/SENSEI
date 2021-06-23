//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_WarpScalar_hxx
#define svtk_m_filter_WarpScalar_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT WarpScalar::WarpScalar(svtkm::FloatDefault scaleAmount)
  : svtkm::filter::FilterField<WarpScalar>()
  , Worklet()
  , NormalFieldName("normal")
  , NormalFieldAssociation(svtkm::cont::Field::Association::ANY)
  , ScalarFactorFieldName("scalarfactor")
  , ScalarFactorFieldAssociation(svtkm::cont::Field::Association::ANY)
  , ScaleAmount(scaleAmount)
{
  this->SetOutputFieldName("warpscalar");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet WarpScalar::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  using vecType = svtkm::Vec<T, 3>;
  svtkm::cont::Field normalF =
    inDataSet.GetField(this->NormalFieldName, this->NormalFieldAssociation);
  svtkm::cont::Field sfF =
    inDataSet.GetField(this->ScalarFactorFieldName, this->ScalarFactorFieldAssociation);
  svtkm::cont::ArrayHandle<vecType> result;
  this->Worklet.Run(field,
                    svtkm::filter::ApplyPolicyFieldOfType<vecType>(normalF, policy, *this),
                    svtkm::filter::ApplyPolicyFieldOfType<T>(sfF, policy, *this),
                    this->ScaleAmount,
                    result);

  return CreateResult(inDataSet, result, this->GetOutputFieldName(), fieldMetadata);
}
}
}
#endif
