//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_ExecutionWholeArray_h
#define svtk_m_exec_ExecutionWholeArray_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>

namespace svtkm
{
namespace exec
{

/// The following classes have been deprecated and are meant to be used
/// internally only. Please use the \c WholeArrayIn, \c WholeArrayOut, and
/// \c WholeArrayInOut \c ControlSignature tags instead.

/// \c ExecutionWholeArray is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure.
///
template <typename T, typename StorageTag, typename DeviceAdapterTag>
class ExecutionWholeArray
{
public:
  using ValueType = T;
  using HandleType = svtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::template ExecutionTypes<DeviceAdapterTag>::Portal;

  SVTKM_CONT
  ExecutionWholeArray()
    : Portal()
  {
  }

  SVTKM_CONT
  ExecutionWholeArray(HandleType& handle)
    : Portal(handle.PrepareForInPlace(DeviceAdapterTag()))
  {
  }

  SVTKM_CONT
  ExecutionWholeArray(HandleType& handle, svtkm::Id length)
    : Portal(handle.PrepareForOutput(length, DeviceAdapterTag()))
  {
  }

  SVTKM_EXEC
  svtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  SVTKM_EXEC
  T Get(svtkm::Id index) const { return this->Portal.Get(index); }

  SVTKM_EXEC
  T operator[](svtkm::Id index) const { return this->Portal.Get(index); }

  SVTKM_EXEC
  void Set(svtkm::Id index, const T& t) const { this->Portal.Set(index, t); }

  SVTKM_EXEC
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
};

/// \c ExecutionWholeArrayConst is an execution object that allows an array handle
/// content to be a parameter in an execution environment
/// function. This can be used to allow worklets to have a shared search
/// structure
///
template <typename T, typename StorageTag, typename DeviceAdapterTag>
class ExecutionWholeArrayConst
{
public:
  using ValueType = T;
  using HandleType = svtkm::cont::ArrayHandle<T, StorageTag>;
  using PortalType = typename HandleType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;

  SVTKM_CONT
  ExecutionWholeArrayConst()
    : Portal()
  {
  }

  SVTKM_CONT
  ExecutionWholeArrayConst(const HandleType& handle)
    : Portal(handle.PrepareForInput(DeviceAdapterTag()))
  {
  }

  SVTKM_EXEC
  svtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  SVTKM_EXEC
  T Get(svtkm::Id index) const { return this->Portal.Get(index); }

  SVTKM_EXEC
  T operator[](svtkm::Id index) const { return this->Portal.Get(index); }

  SVTKM_EXEC
  const PortalType& GetPortal() const { return this->Portal; }

private:
  PortalType Portal;
};
}
} // namespace svtkm::exec

#endif //svtk_m_exec_ExecutionWholeArray_h
