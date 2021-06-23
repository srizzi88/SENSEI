//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagArrayOut_h
#define svtk_m_cont_arg_TransportTagArrayOut_h

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for output arrays.
///
/// \c TransportTagArrayOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for output data.
///
struct TransportTagArrayOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagArrayOut, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that
  // is not an array handle as an argument that is expected to be one.
  SVTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ExecObjectType =
    decltype(std::declval<ContObjectType>().PrepareForOutput(svtkm::Id{}, Device()));

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType operator()(ContObjectType& object,
                                      const InputDomainType& svtkmNotUsed(inputDomain),
                                      svtkm::Id svtkmNotUsed(inputRange),
                                      svtkm::Id outputRange) const
  {
    return object.PrepareForOutput(outputRange, Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagArrayOut_h
