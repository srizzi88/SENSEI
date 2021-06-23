//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_internal_ResolveFieldTypeAndMap_h
#define svtk_m_filter_internal_ResolveFieldTypeAndMap_h

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

template <typename Derived, typename DerivedPolicy>
struct ResolveFieldTypeAndMap
{
  using Self = ResolveFieldTypeAndMap<Derived, DerivedPolicy>;

  Derived* DerivedClass;
  svtkm::cont::DataSet& InputResult;
  const svtkm::filter::FieldMetadata& Metadata;
  const svtkm::filter::PolicyBase<DerivedPolicy>& Policy;
  bool& RanProperly;

  ResolveFieldTypeAndMap(Derived* derivedClass,
                         svtkm::cont::DataSet& inResult,
                         const svtkm::filter::FieldMetadata& fieldMeta,
                         const svtkm::filter::PolicyBase<DerivedPolicy>& policy,
                         bool& ran)
    : DerivedClass(derivedClass)
    , InputResult(inResult)
    , Metadata(fieldMeta)
    , Policy(policy)
    , RanProperly(ran)
  {
  }

  template <typename T, typename StorageTag>
  void operator()(const svtkm::cont::ArrayHandle<T, StorageTag>& field) const
  {
    this->RanProperly =
      this->DerivedClass->DoMapField(this->InputResult, field, this->Metadata, this->Policy);
  }

private:
  void operator=(const ResolveFieldTypeAndMap<Derived, DerivedPolicy>&) = delete;
};
}
}
} // namespace svtkm::filter::internal

#endif //svtk_m_filter_internal_ResolveFieldTypeAndMap_h
