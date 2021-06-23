//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagWholeArrayIn_h
#define svtk_m_cont_arg_TransportTagWholeArrayIn_h

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/arg/Transport.h>

#include <svtkm/exec/ExecutionWholeArray.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for in-place arrays with random access.
///
/// \c TransportTagWholeArrayIn is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for input data.
///
/// The worklet will have random access to the array through a portal
/// interface.
///
struct TransportTagWholeArrayIn
{
};

template <typename ContObjectType, typename Device>
struct Transport<svtkm::cont::arg::TransportTagWholeArrayIn, ContObjectType, Device>
{
  // If you get a compile error here, it means you tried to use an object that
  // is not an array handle as an argument that is expected to be one.
  SVTKM_IS_ARRAY_HANDLE(ContObjectType);

  using ValueType = typename ContObjectType::ValueType;
  using StorageTag = typename ContObjectType::StorageTag;

  using ExecObjectType = svtkm::exec::ExecutionWholeArrayConst<ValueType, StorageTag, Device>;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(ContObjectType& array, const InputDomainType&, svtkm::Id, svtkm::Id) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.

    return ExecObjectType(array);
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagWholeArrayIn_h
