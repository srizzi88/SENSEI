//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_internal_ResolveFieldTypeAndExecute_h
#define svtk_m_filter_internal_ResolveFieldTypeAndExecute_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/filter/FieldMetadata.h>
#include <svtkm/filter/PolicyBase.h>

namespace svtkm
{
namespace filter
{
namespace internal
{

struct ResolveFieldTypeAndExecute
{
  template <typename T,
            typename Storage,
            typename DerivedClass,
            typename DerivedPolicy,
            typename ResultType>
  void operator()(const svtkm::cont::ArrayHandle<T, Storage>& field,
                  DerivedClass* derivedClass,
                  const svtkm::cont::DataSet& inputData,
                  const svtkm::filter::FieldMetadata& fieldMeta,
                  svtkm::filter::PolicyBase<DerivedPolicy> policy,
                  ResultType& result)
  {
    result = derivedClass->DoExecute(inputData, field, fieldMeta, policy);
  }
};
}
}
} // namespace svtkm::filter::internal

#endif //svtk_m_filter_internal_ResolveFieldTypeAndExecute_h
