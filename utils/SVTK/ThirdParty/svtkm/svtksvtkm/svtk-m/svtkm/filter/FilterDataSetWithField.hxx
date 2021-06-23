//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/FieldMetadata.h>
#include <svtkm/filter/FilterTraits.h>
#include <svtkm/filter/PolicyDefault.h>

#include <svtkm/filter/internal/ResolveFieldTypeAndExecute.h>
#include <svtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <svtkm/cont/Error.h>
#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorExecution.h>

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

namespace svtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT FilterDataSetWithField<Derived>::FilterDataSetWithField()
  : OutputFieldName()
  , CoordinateSystemIndex(0)
  , ActiveFieldName()
  , ActiveFieldAssociation(svtkm::cont::Field::Association::ANY)
  , UseCoordinateSystemAsField(false)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT FilterDataSetWithField<Derived>::~FilterDataSetWithField()
{
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (this->UseCoordinateSystemAsField)
  {
    // we need to state that the field is actually a coordinate system, so that
    // the filter uses the proper policy to convert the types.
    return this->PrepareForExecution(input, input.GetCoordinateSystem(), policy);
  }
  else
  {
    return this->PrepareForExecution(
      input, input.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation()), policy);
  }
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::Field& field,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  svtkm::filter::FieldMetadata metaData(field);
  svtkm::cont::DataSet result;

  svtkm::cont::CastAndCall(
    svtkm::filter::ApplyPolicyFieldActive(field, policy, svtkm::filter::FilterTraits<Derived>()),
    internal::ResolveFieldTypeAndExecute(),
    static_cast<Derived*>(this),
    input,
    metaData,
    policy,
    result);
  return result;
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet FilterDataSetWithField<Derived>::PrepareForExecution(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::CoordinateSystem& field,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //We have a special signature just for CoordinateSystem, so that we can ask
  //the policy for the storage types and value types just for coordinate systems
  svtkm::filter::FieldMetadata metaData(field);
  svtkm::cont::DataSet result;

  //determine the field type first
  using Traits = svtkm::filter::FilterTraits<Derived>;
  constexpr bool supportsVec3 =
    svtkm::ListHas<typename Traits::InputFieldTypeList, svtkm::Vec3f>::value;
  using supportsCoordinateSystem = std::integral_constant<bool, supportsVec3>;
  svtkm::cont::ConditionalCastAndCall(supportsCoordinateSystem(),
                                     field,
                                     internal::ResolveFieldTypeAndExecute(),
                                     static_cast<Derived*>(this),
                                     input,
                                     metaData,
                                     policy,
                                     result);

  return result;
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT bool FilterDataSetWithField<Derived>::MapFieldOntoOutput(
  svtkm::cont::DataSet& result,
  const svtkm::cont::Field& field,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  bool valid = false;

  svtkm::filter::FieldMetadata metaData(field);
  using FunctorType = internal::ResolveFieldTypeAndMap<Derived, DerivedPolicy>;
  FunctorType functor(static_cast<Derived*>(this), result, metaData, policy, valid);

  svtkm::cont::CastAndCall(svtkm::filter::ApplyPolicyFieldNotActive(field, policy), functor);

  //the bool valid will be modified by the map algorithm to hold if the
  //mapping occurred or not. If the mapping was good a new field has been
  //added to the result that was passed in.
  return valid;
}
}
}
