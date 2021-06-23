//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#ifndef svtk_m_filter_ComputeMoments_hxx
#define svtk_m_filter_ComputeMoments_hxx

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/worklet/moments/ComputeMoments.h>

namespace svtkm
{
namespace filter
{

SVTKM_CONT ComputeMoments::ComputeMoments()
{
  this->SetOutputFieldName("moments_");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet ComputeMoments::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (fieldMetadata.GetAssociation() != svtkm::cont::Field::Association::POINTS)
  {
    throw svtkm::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  svtkm::cont::DataSet output;
  output.CopyStructure(input);

  auto worklet = svtkm::worklet::moments::ComputeMoments(this->Spacing, this->Radius);

  worklet.Run(input.GetCellSet(), field, this->Order, output);

  return output;
}
}
}
#endif //svtk_m_filter_ComputeMoments_hxx
