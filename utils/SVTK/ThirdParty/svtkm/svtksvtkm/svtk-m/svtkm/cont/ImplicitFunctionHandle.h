//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ImplicitFunctionHandle_h
#define svtk_m_cont_ImplicitFunctionHandle_h

#include <svtkm/ImplicitFunction.h>
#include <svtkm/cont/VirtualObjectHandle.h>

namespace svtkm
{
namespace cont
{

class SVTKM_ALWAYS_EXPORT ImplicitFunctionHandle
  : public svtkm::cont::VirtualObjectHandle<svtkm::ImplicitFunction>
{
private:
  using Superclass = svtkm::cont::VirtualObjectHandle<svtkm::ImplicitFunction>;

public:
  ImplicitFunctionHandle() = default;

  template <typename ImplicitFunctionType,
            typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit ImplicitFunctionHandle(ImplicitFunctionType* function,
                                  bool acquireOwnership = true,
                                  DeviceAdapterList devices = DeviceAdapterList())
    : Superclass(function, acquireOwnership, devices)
  {
  }
};

template <typename ImplicitFunctionType,
          typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
SVTKM_CONT ImplicitFunctionHandle
make_ImplicitFunctionHandle(ImplicitFunctionType&& func,
                            DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<ImplicitFunctionType>::type;
  return ImplicitFunctionHandle(
    new IFType(std::forward<ImplicitFunctionType>(func)), true, devices);
}

template <typename ImplicitFunctionType, typename... Args>
SVTKM_CONT ImplicitFunctionHandle make_ImplicitFunctionHandle(Args&&... args)
{
  return ImplicitFunctionHandle(new ImplicitFunctionType(std::forward<Args>(args)...),
                                true,
                                SVTKM_DEFAULT_DEVICE_ADAPTER_LIST());
}

template <typename ImplicitFunctionType, typename DeviceAdapterList, typename... Args>
SVTKM_CONT ImplicitFunctionHandle make_ImplicitFunctionHandle(Args&&... args)
{
  return ImplicitFunctionHandle(
    new ImplicitFunctionType(std::forward<Args>(args)...), true, DeviceAdapterList());
}

//============================================================================
/// A helpful wrapper that returns a functor that calls the (virtual) value method of a given
/// ImplicitFunction. Can be passed to things that expect a functor instead of an ImplictFunction
/// class (like an array transform).
///
class SVTKM_ALWAYS_EXPORT ImplicitFunctionValueHandle
  : public svtkm::cont::ExecutionAndControlObjectBase
{
  svtkm::cont::ImplicitFunctionHandle Handle;

public:
  ImplicitFunctionValueHandle() = default;

  ImplicitFunctionValueHandle(const ImplicitFunctionHandle& handle)
    : Handle(handle)
  {
  }

  template <typename ImplicitFunctionType,
            typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit ImplicitFunctionValueHandle(ImplicitFunctionType* function,
                                       bool acquireOwnership = true,
                                       DeviceAdapterList devices = DeviceAdapterList())
    : Handle(function, acquireOwnership, devices)
  {
  }

  SVTKM_CONT const svtkm::cont::ImplicitFunctionHandle& GetHandle() const { return this->Handle; }

  SVTKM_CONT
  svtkm::ImplicitFunctionValue PrepareForExecution(svtkm::cont::DeviceAdapterId device) const
  {
    return svtkm::ImplicitFunctionValue(this->Handle.PrepareForExecution(device));
  }

  SVTKM_CONT svtkm::ImplicitFunctionValue PrepareForControl() const
  {
    return svtkm::ImplicitFunctionValue(this->Handle.PrepareForControl());
  }
};

template <typename ImplicitFunctionType,
          typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
SVTKM_CONT ImplicitFunctionValueHandle
make_ImplicitFunctionValueHandle(ImplicitFunctionType&& func,
                                 DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<ImplicitFunctionType>::type;
  return ImplicitFunctionValueHandle(
    new IFType(std::forward<ImplicitFunctionType>(func)), true, devices);
}

template <typename ImplicitFunctionType, typename... Args>
SVTKM_CONT ImplicitFunctionValueHandle make_ImplicitFunctionValueHandle(Args&&... args)
{
  return ImplicitFunctionValueHandle(new ImplicitFunctionType(std::forward<Args>(args)...),
                                     true,
                                     SVTKM_DEFAULT_DEVICE_ADAPTER_LIST());
}

template <typename ImplicitFunctionType, typename DeviceAdapterList, typename... Args>
SVTKM_CONT ImplicitFunctionValueHandle make_ImplicitFunctionValueHandle(Args&&... args)
{
  return ImplicitFunctionValueHandle(
    new ImplicitFunctionType(std::forward<Args>(args)...), true, DeviceAdapterList());
}
}
} // svtkm::cont

#endif // svtk_m_cont_ImplicitFunctionHandle_h
