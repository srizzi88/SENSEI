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

namespace svtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT FilterDataSet<Derived>::FilterDataSet()
  : CoordinateSystemIndex(0)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT FilterDataSet<Derived>::~FilterDataSet()
{
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet FilterDataSet<Derived>::PrepareForExecution(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  return (static_cast<Derived*>(this))->DoExecute(input, policy);
}

//-----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT bool FilterDataSet<Derived>::MapFieldOntoOutput(
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
