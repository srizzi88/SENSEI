//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagArrayIn_h
#define svtk_m_cont_arg_TransportTagArrayIn_h

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
struct TransportTagArrayIn
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagArrayIn, ContObjectType, Device>
{
  SVTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInput(Device()));

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType operator()(const ContObjectType& object,
                                      const InputDomainType& svtkmNotUsed(inputDomain),
                                      svtkm::Id inputRange,
                                      svtkm::Id svtkmNotUsed(outputRange)) const
  {
    if (object.GetNumberOfValues() != inputRange)
    {
      throw svtkm::cont::ErrorBadValue("Input array to worklet invocation the wrong size.");
    }

    return object.PrepareForInput(Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagArrayIn_h
