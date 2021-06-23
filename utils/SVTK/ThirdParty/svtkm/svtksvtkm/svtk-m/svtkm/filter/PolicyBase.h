//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_PolicyBase_h
#define svtk_m_filter_PolicyBase_h

#include <svtkm/List.h>

#include <svtkm/cont/CellSetList.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DeviceAdapterList.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/StorageList.h>

#include <svtkm/filter/FilterTraits.h>

namespace svtkm
{
namespace filter
{

template <typename Derived>
struct PolicyBase
{
  using FieldTypeList = SVTKM_DEFAULT_TYPE_LIST;
  using StorageList = svtkm::ListAppend<
    SVTKM_DEFAULT_STORAGE_LIST,
    svtkm::List<
      svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
      svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::Float32>,
                                              svtkm::cont::ArrayHandle<svtkm::Float32>,
                                              svtkm::cont::ArrayHandle<svtkm::Float32>>::StorageTag,
      svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<svtkm::Float64>,
                                              svtkm::cont::ArrayHandle<svtkm::Float64>,
                                              svtkm::cont::ArrayHandle<svtkm::Float64>>::StorageTag>>;

  using StructuredCellSetList = svtkm::cont::CellSetListStructured;
  using UnstructuredCellSetList = svtkm::cont::CellSetListUnstructured;
  using AllCellSetList = SVTKM_DEFAULT_CELL_SET_LIST;
};

namespace internal
{

namespace detail
{

// Given a base type, forms a list of all types with the same Vec structure but with the
// base component replaced with each of the basic C types.
template <typename BaseType>
struct AllCastingTypes
{
  using VTraits = svtkm::VecTraits<BaseType>;

  using type = svtkm::List<typename VTraits::template ReplaceBaseComponentType<svtkm::Int8>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::UInt8>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::Int16>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::UInt8>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::Int32>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::UInt32>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::Int64>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::UInt64>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::Float32>,
                          typename VTraits::template ReplaceBaseComponentType<svtkm::Float64>>;
};

// Provides a transform template that builds a cast from an array of some source type to a
// cast array to a specific target type.
template <typename TargetT, typename Storage>
struct CastArrayTransform
{
  template <typename SourceT>
  using Transform = svtkm::cont::ArrayHandleCast<TargetT, svtkm::cont::ArrayHandle<SourceT, Storage>>;
};

// Provides a predicate for a particular storage that resolves to std::true_type if a given
// type cannot be used with the storage.
template <typename Storage>
struct ArrayValidPredicate
{
  template <typename T>
  using Predicate = svtkm::cont::internal::IsInValidArrayHandle<T, Storage>;
};

template <typename TargetT, typename Storage, bool Valid>
struct AllCastArraysForStorageImpl;

template <typename TargetT, typename Storage>
struct ValidCastingTypes
{
  using type = svtkm::ListRemoveIf<typename AllCastingTypes<TargetT>::type,
                                  ArrayValidPredicate<Storage>::template Predicate>;
};

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, true>
{
  using SourceTypes = typename ValidCastingTypes<TargetT, Storage>::type;
  using CastArrays =
    svtkm::ListTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>;
  using type = svtkm::ListAppend<svtkm::List<svtkm::cont::ArrayHandle<TargetT, Storage>>, CastArrays>;
};

template <typename TargetT, typename Storage>
struct AllCastArraysForStorageImpl<TargetT, Storage, false>
{
  using SourceTypes = typename ValidCastingTypes<TargetT, Storage>::type;
  using type =
    svtkm::ListTransform<SourceTypes, CastArrayTransform<TargetT, Storage>::template Transform>;
};

// Special cases for known storage with limited type support.
template <>
struct AllCastArraysForStorageImpl<svtkm::Vec3f,
                                   svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   true>
{
  using type = svtkm::List<svtkm::cont::ArrayHandleUniformPointCoordinates>;
};
template <typename T>
struct AllCastArraysForStorageImpl<svtkm::Vec<T, 3>,
                                   svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = svtkm::List<svtkm::cont::ArrayHandleCast<
    svtkm::Vec<T, 3>,
    svtkm::cont::ArrayHandle<svtkm::Vec3f,
                            svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag>>>;
};
template <typename TargetT>
struct AllCastArraysForStorageImpl<TargetT,
                                   svtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                                   false>
{
  using type = svtkm::ListEmpty;
};

template <typename T, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<svtkm::Vec<T, 3>,
                                   svtkm::cont::StorageTagCartesianProduct<S1, S2, S3>,
                                   true>
{
  using type = svtkm::List<svtkm::cont::ArrayHandleCartesianProduct<svtkm::cont::ArrayHandle<T, S1>,
                                                                  svtkm::cont::ArrayHandle<T, S2>,
                                                                  svtkm::cont::ArrayHandle<T, S3>>>;
};
template <typename TargetT, typename S1, typename S2, typename S3>
struct AllCastArraysForStorageImpl<TargetT,
                                   svtkm::cont::StorageTagCartesianProduct<S1, S2, S3>,
                                   false>
{
  using type = svtkm::ListEmpty;
};

// Given a target type and storage of an array handle, provides a list this array handle plus all
// array handles that can be cast to the target type wrapped in an ArrayHandleCast that does so.
template <typename TargetT, typename Storage>
struct AllCastArraysForStorage
{
  using type = typename AllCastArraysForStorageImpl<
    TargetT,
    Storage,
    svtkm::cont::internal::IsValidArrayHandle<TargetT, Storage>::value>::type;
};

// Provides a transform template that converts a storage type to a list of all arrays that come
// from that storage type and can be cast to a target type (wrapped in an ArrayHandleCast as
// appropriate).
template <typename TargetT>
struct AllCastArraysTransform
{
  template <typename Storage>
  using Transform = typename AllCastArraysForStorage<TargetT, Storage>::type;
};

// Given a target type and a list of storage types, provides a joined list of all possible arrays
// of any of these storage cast to the target type.
template <typename TargetT, typename StorageList>
struct AllCastArraysForStorageList
{
  SVTKM_IS_LIST(StorageList);
  using listOfLists =
    svtkm::ListTransform<StorageList, AllCastArraysTransform<TargetT>::template Transform>;
  using type = svtkm::ListApply<listOfLists, svtkm::ListAppend>;
};

} // detail

template <typename TargetT, typename StorageList>
using ArrayHandleMultiplexerForStorageList = svtkm::cont::ArrayHandleMultiplexerFromList<
  typename detail::AllCastArraysForStorageList<TargetT, StorageList>::type>;

} // namespace internal

//-----------------------------------------------------------------------------
/// \brief Get an array from a `Field` that is not the active field.
///
/// Use this form for getting a `Field` when you don't know the type and it is not
/// (necessarily) the "active" field of the filter. It is generally used for arrays
/// passed to the `DoMapField` method of filters.
///
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::VariantArrayHandleBase<typename DerivedPolicy::FieldTypeList>
ApplyPolicyFieldNotActive(const svtkm::cont::Field& field, svtkm::filter::PolicyBase<DerivedPolicy>)
{
  using TypeList = typename DerivedPolicy::FieldTypeList;
  return field.GetData().ResetTypes(TypeList());
}

//-----------------------------------------------------------------------------
/// \brief Get an `ArrayHandle` of a specific type from a `Field`.
///
/// Use this form of `ApplyPolicy` when you know what the value type of a field is or
/// (more likely) there is a type you are going to cast it to anyway.
///
template <typename T, typename DerivedPolicy, typename FilterType>
SVTKM_CONT internal::ArrayHandleMultiplexerForStorageList<
  T,
  svtkm::ListAppend<typename svtkm::filter::FilterTraits<FilterType>::AdditionalFieldStorage,
                   typename DerivedPolicy::StorageList>>
ApplyPolicyFieldOfType(const svtkm::cont::Field& field,
                       svtkm::filter::PolicyBase<DerivedPolicy>,
                       const FilterType&)
{
  using ArrayHandleMultiplexerType = internal::ArrayHandleMultiplexerForStorageList<
    T,
    svtkm::ListAppend<typename FilterType::AdditionalFieldStorage,
                     typename DerivedPolicy::StorageList>>;
  return field.GetData().AsMultiplexer<ArrayHandleMultiplexerType>();
}

//-----------------------------------------------------------------------------
/// \brief Get an array from a `Field` that follows the types of an active field.
///
/// Use this form for getting a `Field` to build the types that are appropriate for
/// the active field of this filter.
///
template <typename DerivedPolicy, typename FilterType>
SVTKM_CONT svtkm::cont::VariantArrayHandleBase<typename svtkm::filter::DeduceFilterFieldTypes<
  DerivedPolicy,
  typename svtkm::filter::FilterTraits<FilterType>::InputFieldTypeList>::TypeList>
ApplyPolicyFieldActive(const svtkm::cont::Field& field,
                       svtkm::filter::PolicyBase<DerivedPolicy>,
                       svtkm::filter::FilterTraits<FilterType>)
{
  using FilterTypes = typename svtkm::filter::FilterTraits<FilterType>::InputFieldTypeList;
  using TypeList =
    typename svtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, FilterTypes>::TypeList;
  return field.GetData().ResetTypes(TypeList());
}

////-----------------------------------------------------------------------------
///// \brief Get an array from a `Field` limited to a given set of types.
/////
//template <typename DerivedPolicy, typename ListOfTypes>
//SVTKM_CONT svtkm::cont::VariantArrayHandleBase<
//  typename svtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, ListOfTypes>::TypeList>
//ApplyPolicyFieldOfTypes(
//  const svtkm::cont::Field& field, svtkm::filter::PolicyBase<DerivedPolicy>, ListOfTypes)
//{
//  using TypeList =
//    typename svtkm::filter::DeduceFilterFieldTypes<DerivedPolicy, ListOfTypes>::TypeList;
//  return field.GetData().ResetTypes(TypeList());
//}

//-----------------------------------------------------------------------------
/// \brief Ge a cell set from a `DynamicCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those types specified in a policy.
///
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::DynamicCellSetBase<typename DerivedPolicy::AllCellSetList> ApplyPolicyCellSet(
  const svtkm::cont::DynamicCellSet& cellset,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::AllCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
/// \brief Get a structured cell set from a `DynamicCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those structured cell set types
/// specified in a policy.
///
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::DynamicCellSetBase<typename DerivedPolicy::StructuredCellSetList>
ApplyPolicyCellSetStructured(const svtkm::cont::DynamicCellSet& cellset,
                             svtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::StructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
/// \brief Get an unstructured cell set from a `DynamicCellSet` object.
///
/// Adjusts the types of `CellSet`s to support those unstructured cell set types
/// specified in a policy.
///
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::DynamicCellSetBase<typename DerivedPolicy::UnstructuredCellSetList>
ApplyPolicyCellSetUnstructured(const svtkm::cont::DynamicCellSet& cellset,
                               svtkm::filter::PolicyBase<DerivedPolicy>)
{
  using CellSetList = typename DerivedPolicy::UnstructuredCellSetList;
  return cellset.ResetCellSetList(CellSetList());
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>
  MakeSerializableField(svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return {};
}

template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>
MakeSerializableField(const svtkm::cont::Field& field, svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return svtkm::cont::SerializableField<typename DerivedPolicy::FieldTypeList>{ field };
}

template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                          typename DerivedPolicy::AllCellSetList>
  MakeSerializableDataSet(svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return {};
}

template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                          typename DerivedPolicy::AllCellSetList>
MakeSerializableDataSet(const svtkm::cont::DataSet& dataset, svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return svtkm::cont::SerializableDataSet<typename DerivedPolicy::FieldTypeList,
                                         typename DerivedPolicy::AllCellSetList>{ dataset };
}
}
} // svtkm::filter

#endif //svtk_m_filter_PolicyBase_h
