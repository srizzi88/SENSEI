//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_AtomicArray_h
#define svtk_m_cont_AtomicArray_h

#include <svtkm/List.h>
#include <svtkm/ListTag.h>
#include <svtkm/StaticAssert.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/ExecutionObjectBase.h>
#include <svtkm/exec/AtomicArrayExecutionObject.h>

namespace svtkm
{
namespace cont
{

/// \brief A type list containing types that can be used with an AtomicArray.
///
using AtomicArrayTypeList = svtkm::List<svtkm::UInt32, svtkm::Int32, svtkm::UInt64, svtkm::Int64>;

struct SVTKM_DEPRECATED(1.6,
                       "AtomicArrayTypeListTag replaced by AtomicArrayTypeList. Note that the "
                       "new AtomicArrayTypeList cannot be subclassed.") AtomicArrayTypeListTag
  : svtkm::internal::ListAsListTag<AtomicArrayTypeList>
{
};


/// A class that can be used to atomically operate on an array of values safely
/// across multiple instances of the same worklet. This is useful when you have
/// an algorithm that needs to accumulate values in parallel, but writing out a
/// value per worklet might be memory prohibitive.
///
/// To construct an AtomicArray you will need to pass in an
/// svtkm::cont::ArrayHandle that is used as the underlying storage for the
/// AtomicArray
///
/// Supported Operations: get / add / compare and swap (CAS). See
/// AtomicArrayExecutionObject for details.
///
/// Supported Types: 32 / 64 bit signed/unsigned integers.
///
///
template <typename T>
class AtomicArray : public svtkm::cont::ExecutionObjectBase
{
  static constexpr bool ValueTypeIsValid = svtkm::ListHas<AtomicArrayTypeList, T>::value;
  SVTKM_STATIC_ASSERT_MSG(ValueTypeIsValid, "AtomicArray used with unsupported ValueType.");


public:
  using ValueType = T;

  SVTKM_CONT
  AtomicArray()
    : Handle(svtkm::cont::ArrayHandle<T>())
  {
  }

  SVTKM_CONT AtomicArray(svtkm::cont::ArrayHandle<T> handle)
    : Handle(handle)
  {
  }

  template <typename Device>
  SVTKM_CONT svtkm::exec::AtomicArrayExecutionObject<T, Device> PrepareForExecution(Device) const
  {
    return svtkm::exec::AtomicArrayExecutionObject<T, Device>(this->Handle);
  }

private:
  svtkm::cont::ArrayHandle<T> Handle;
};
}
} // namespace svtkm::exec

#endif //svtk_m_cont_AtomicArray_h
