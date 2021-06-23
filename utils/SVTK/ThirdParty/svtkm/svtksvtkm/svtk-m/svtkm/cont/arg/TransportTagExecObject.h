//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagExecObject_h
#define svtk_m_cont_arg_TransportTagExecObject_h

#include <svtkm/Types.h>

#include <svtkm/cont/arg/Transport.h>

#include <svtkm/cont/ExecutionObjectBase.h>


namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for execution objects.
///
/// \c TransportTagExecObject is a tag used with the \c Transport class to
/// transport objects that work directly in the execution environment.
///
struct TransportTagExecObject
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagExecObject, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that is not an execution
  // object as an argument that is expected to be one. All execution objects are expected to
  // inherit from svtkm::cont::ExecutionObjectBase and have a PrepareForExecution method.
  SVTKM_IS_EXECUTION_OBJECT(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForExecution(Device()));
  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(ContObjectType& object, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    return object.PrepareForExecution(Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagExecObject_h
