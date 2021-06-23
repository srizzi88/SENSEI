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
#include <svtkm/filter/PolicyDefault.h>

#include <svtkm/filter/internal/ResolveFieldTypeAndExecute.h>
#include <svtkm/filter/internal/ResolveFieldTypeAndMap.h>

#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/Logging.h>

namespace svtkm
{
namespace filter
{

namespace internal
{

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPreExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PreExecute(
              std::declval<InputType>(),
              std::declval<svtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPostExecute
{
  template <typename U,
            typename S = decltype(std::declval<U>().PostExecute(
              std::declval<InputType>(),
              std::declval<InputType&>(),
              std::declval<svtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};


template <typename T, typename InputType, typename DerivedPolicy>
struct SupportsPrepareForExecution
{
  template <typename U,
            typename S = decltype(std::declval<U>().PrepareForExecution(
              std::declval<InputType>(),
              std::declval<svtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

template <typename T, typename DerivedPolicy>
struct SupportsMapFieldOntoOutput
{
  template <typename U,
            typename S = decltype(std::declval<U>().MapFieldOntoOutput(
              std::declval<svtkm::cont::DataSet&>(),
              std::declval<svtkm::cont::Field>(),
              std::declval<svtkm::filter::PolicyBase<DerivedPolicy>>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<T>(0));
};

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::true_type, Derived* self, Args&&... args)
{
  return self->PreExecute(std::forward<Args>(args)...);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPreExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPreExecute(Derived* self,
                    const InputType& input,
                    const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPreExecute<Derived, InputType, DerivedPolicy>::type;
  CallPreExecuteInternal(call_supported_t(), self, input, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutputInternal(std::true_type,
                                    Derived* self,
                                    const svtkm::cont::DataSet& input,
                                    svtkm::cont::DataSet& output,
                                    const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  for (svtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (self->GetFieldsToPass().IsFieldSelected(field))
    {
      self->MapFieldOntoOutput(output, field, policy);
    }
  }
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutputInternal(std::false_type,
                                    Derived* self,
                                    const svtkm::cont::DataSet& input,
                                    svtkm::cont::DataSet& output,
                                    const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  // no MapFieldOntoOutput method is present. In that case, we simply copy the
  // requested input fields to the output.
  for (svtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (self->GetFieldsToPass().IsFieldSelected(field))
    {
      output.AddField(field);
    }
  }
}

//--------------------------------------------------------------------------------
template <typename Derived, typename DerivedPolicy>
void CallMapFieldOntoOutput(Derived* self,
                            const svtkm::cont::DataSet& input,
                            svtkm::cont::DataSet& output,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  using call_supported_t = typename SupportsMapFieldOntoOutput<Derived, DerivedPolicy>::type;
  CallMapFieldOntoOutputInternal(call_supported_t(), self, input, output, policy);
}

//--------------------------------------------------------------------------------
// forward declare.
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecution(Derived* self,
                                  const InputType& input,
                                  const svtkm::filter::PolicyBase<DerivedPolicy>& policy);

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecutionInternal(std::true_type,
                                          Derived* self,
                                          const InputType& input,
                                          const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  return self->PrepareForExecution(input, policy);
}

//--------------------------------------------------------------------------------
// specialization for PartitionedDataSet input when `PrepareForExecution` is not provided
// by the subclass. we iterate over blocks and execute for each block
// individually.
template <typename Derived, typename DerivedPolicy>
svtkm::cont::PartitionedDataSet CallPrepareForExecutionInternal(
  std::false_type,
  Derived* self,
  const svtkm::cont::PartitionedDataSet& input,
  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  svtkm::cont::PartitionedDataSet output;
  for (const auto& inBlock : input)
  {
    svtkm::cont::DataSet outBlock = CallPrepareForExecution(self, inBlock, policy);
    CallMapFieldOntoOutput(self, inBlock, outBlock, policy);
    output.AppendPartition(outBlock);
  }
  return output;
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
InputType CallPrepareForExecution(Derived* self,
                                  const InputType& input,
                                  const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t =
    typename SupportsPrepareForExecution<Derived, InputType, DerivedPolicy>::type;
  return CallPrepareForExecutionInternal(call_supported_t(), self, input, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecuteInternal(std::true_type,
                             Derived* self,
                             const InputType& input,
                             InputType& output,
                             const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  self->PostExecute(input, output, policy);
}

//--------------------------------------------------------------------------------
template <typename Derived, typename... Args>
void CallPostExecuteInternal(std::false_type, Derived*, Args&&...)
{
}

//--------------------------------------------------------------------------------
template <typename Derived, typename InputType, typename DerivedPolicy>
void CallPostExecute(Derived* self,
                     const InputType& input,
                     InputType& output,
                     const svtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  using call_supported_t = typename SupportsPostExecute<Derived, InputType, DerivedPolicy>::type;
  CallPostExecuteInternal(call_supported_t(), self, input, output, policy);
}
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT Filter<Derived>::Filter()
  : Invoke()
  , FieldsToPass(svtkm::filter::FieldSelection::MODE_ALL)
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT Filter<Derived>::~Filter()
{
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT svtkm::cont::DataSet Filter<Derived>::Execute(const svtkm::cont::DataSet& input)
{
  return this->Execute(input, svtkm::filter::PolicyDefault());
}

//----------------------------------------------------------------------------
template <typename Derived>
inline SVTKM_CONT svtkm::cont::PartitionedDataSet Filter<Derived>::Execute(
  const svtkm::cont::PartitionedDataSet& input)
{
  return this->Execute(input, svtkm::filter::PolicyDefault());
}


//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::DataSet Filter<Derived>::Execute(
  const svtkm::cont::DataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  SVTKM_LOG_SCOPE(
    svtkm::cont::LogLevel::Perf, "Filter: '%s'", svtkm::cont::TypeToString<Derived>().c_str());

  Derived* self = static_cast<Derived*>(this);
  svtkm::cont::PartitionedDataSet output =
    self->Execute(svtkm::cont::PartitionedDataSet(input), policy);
  if (output.GetNumberOfPartitions() > 1)
  {
    throw svtkm::cont::ErrorFilterExecution("Expecting at most 1 block.");
  }
  return output.GetNumberOfPartitions() == 1 ? output.GetPartition(0) : svtkm::cont::DataSet();
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
SVTKM_CONT svtkm::cont::PartitionedDataSet Filter<Derived>::Execute(
  const svtkm::cont::PartitionedDataSet& input,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  SVTKM_LOG_SCOPE(svtkm::cont::LogLevel::Perf,
                 "Filter (PartitionedDataSet): '%s'",
                 svtkm::cont::TypeToString<Derived>().c_str());

  Derived* self = static_cast<Derived*>(this);

  // Call `void Derived::PreExecute<DerivedPolicy>(input, policy)`, if defined.
  internal::CallPreExecute(self, input, policy);

  // Call `PrepareForExecution` (which should probably be renamed at some point)
  svtkm::cont::PartitionedDataSet output = internal::CallPrepareForExecution(self, input, policy);

  // Call `Derived::PostExecute<DerivedPolicy>(input, output, policy)` if defined.
  internal::CallPostExecute(self, input, output, policy);
  return output;
}

//----------------------------------------------------------------------------
template <typename Derived>
template <typename DerivedPolicy>
inline SVTKM_CONT void Filter<Derived>::MapFieldsToPass(
  const svtkm::cont::DataSet& input,
  svtkm::cont::DataSet& output,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  Derived* self = static_cast<Derived*>(this);
  for (svtkm::IdComponent cc = 0; cc < input.GetNumberOfFields(); ++cc)
  {
    auto field = input.GetField(cc);
    if (this->GetFieldsToPass().IsFieldSelected(field))
    {
      internal::CallMapFieldOntoOutput(self, output, field, policy);
    }
  }
}
}
}
