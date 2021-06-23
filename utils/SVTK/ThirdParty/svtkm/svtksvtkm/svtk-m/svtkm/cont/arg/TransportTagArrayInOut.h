//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagArrayInOut_h
#define svtk_m_cont_arg_TransportTagArrayInOut_h

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/arg/Transport.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for in-place arrays.
///
/// \c TransportTagArrayInOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for data that is both input and output
/// (that is, in place modification of array data).
///
struct TransportTagArrayInOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagArrayInOut, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that
  // is not an array handle as an argument that is expected to be one.
  SVTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ExecObjectType = decltype(std::declval<ContObjectType>().PrepareForInPlace(Device()));

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType operator()(ContObjectType& object,
                                      const InputDomainType& svtkmNotUsed(inputDomain),
                                      svtkm::Id svtkmNotUsed(inputRange),
                                      svtkm::Id outputRange) const
  {
    if (object.GetNumberOfValues() != outputRange)
    {
      throw svtkm::cont::ErrorBadValue("Input/output array to worklet invocation the wrong size.");
    }

    return object.PrepareForInPlace(Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagArrayInOut_h
