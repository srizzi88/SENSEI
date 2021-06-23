//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_arg_TransportTagAtomicArray_h
#define svtk_m_cont_arg_TransportTagAtomicArray_h

#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/StorageBasic.h>
#include <svtkm/cont/StorageVirtual.h>


#include <svtkm/cont/arg/Transport.h>

#include <svtkm/cont/AtomicArray.h>

namespace svtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for in-place arrays with atomic operations.
///
/// \c TransportTagAtomicArray is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for data that is both input and output
/// (that is, in place modification of array data). The array will be wrapped
/// in a svtkm::exec::AtomicArray class that provides atomic operations (like
/// add and compare/swap).
///
struct TransportTagAtomicArray
{
};

template <typename T, typename Device>
struct Transport<svtkm::cont::arg::TransportTagAtomicArray,
                 svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>,
                 Device>
{
  using ExecObjectType = svtkm::exec::AtomicArrayExecutionObject<T, Device>;
  using ExecType = svtkm::cont::AtomicArray<T>;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>& array,
             const InputDomainType&,
             svtkm::Id,
             svtkm::Id) const
  {
    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.
    ExecType obj(array);
    return obj.PrepareForExecution(Device());
  }
};

template <typename T, typename Device>
struct Transport<svtkm::cont::arg::TransportTagAtomicArray,
                 svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>,
                 Device>
{
  using ExecObjectType = svtkm::exec::AtomicArrayExecutionObject<T, Device>;
  using ExecType = svtkm::cont::AtomicArray<T>;

  template <typename InputDomainType>
  SVTKM_CONT ExecObjectType
  operator()(svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagVirtual>& array,
             const InputDomainType&,
             svtkm::Id,
             svtkm::Id) const
  {
    using ArrayHandleType = svtkm::cont::ArrayHandle<T>;
    const bool is_type = svtkm::cont::IsType<ArrayHandleType>(array);
    if (!is_type)
    {
#if defined(SVTKM_ENABLE_LOGGING)
      SVTKM_LOG_CAST_FAIL(array, ArrayHandleType);
#endif
      throw svtkm::cont::ErrorBadValue("Arrays being used as atomic's must always have storage that "
                                      "is of the type StorageTagBasic.");
    }

    ArrayHandleType handle = svtkm::cont::Cast<ArrayHandleType>(array);

    // Note: we ignore the size of the domain because the randomly accessed
    // array might not have the same size depending on how the user is using
    // the array.
    ExecType obj(handle);
    return obj.PrepareForExecution(Device());
  }
};
}
}
} // namespace svtkm::cont::arg

#endif //svtk_m_cont_arg_TransportTagAtomicArray_h
