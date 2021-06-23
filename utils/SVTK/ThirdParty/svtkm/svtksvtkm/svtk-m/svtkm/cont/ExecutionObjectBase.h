//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ExecutionObjectBase_h
#define svtk_m_cont_ExecutionObjectBase_h

#include <svtkm/Types.h>

#include <svtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

namespace svtkm
{
namespace cont
{

/// Base \c ExecutionObjectBase for execution objects to inherit from so that
/// you can use an arbitrary object as a parameter in an execution environment
/// function. Any subclass of \c ExecutionObjectBase must implement a
/// \c PrepareForExecution method that takes a device adapter tag and returns
/// an object for that device.
///
struct ExecutionObjectBase
{
};

namespace internal
{

namespace detail
{

struct CheckPrepareForExecution
{
  template <typename T>
  static auto check(T* p)
    -> decltype(p->PrepareForExecution(svtkm::cont::DeviceAdapterTagSerial{}), std::true_type());

  template <typename T>
  static auto check(...) -> std::false_type;
};

} // namespace detail

template <typename T>
using IsExecutionObjectBase =
  std::is_base_of<svtkm::cont::ExecutionObjectBase, typename std::decay<T>::type>;

template <typename T>
struct HasPrepareForExecution
  : decltype(detail::CheckPrepareForExecution::check<typename std::decay<T>::type>(nullptr))
{
};

} // namespace internal
}
} // namespace svtkm::cont

/// Checks that the argument is a proper execution object.
///
#define SVTKM_IS_EXECUTION_OBJECT(execObject)                                                       \
  static_assert(::svtkm::cont::internal::IsExecutionObjectBase<execObject>::value,                  \
                "Provided type is not a subclass of svtkm::cont::ExecutionObjectBase.");            \
  static_assert(::svtkm::cont::internal::HasPrepareForExecution<execObject>::value,                 \
                "Provided type does not have requisite PrepareForExecution method.")

#endif //svtk_m_cont_ExecutionObjectBase_h
