//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_WarpVector_hxx
#define svtk_m_filter_WarpVector_hxx

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT WarpVector::WarpVector(svtkm::FloatDefault scale)
  : svtkm::filter::FilterField<WarpVector>()
  , Worklet()
  , VectorFieldName("normal")
  , VectorFieldAssociation(svtkm::cont::Field::Association::ANY)
  , Scale(scale)
{
  this->SetOutputFieldName("warpvector");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet WarpVector::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  using vecType = svtkm::Vec<T, 3>;
  svtkm::cont::Field vectorF =
    inDataSet.GetField(this->VectorFieldName, this->VectorFieldAssociation);
  svtkm::cont::ArrayHandle<vecType> result;
  this->Worklet.Run(field,
                    svtkm::filter::ApplyPolicyFieldOfType<vecType>(vectorF, policy, *this),
                    this->Scale,
                    result);

  return CreateResult(inDataSet, result, this->GetOutputFieldName(), fieldMetadata);
}
}
}
#endif
