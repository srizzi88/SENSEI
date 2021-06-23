//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_interop_TransferToOpenGL_h
#define svtk_m_interop_TransferToOpenGL_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/interop/BufferState.h>
#include <svtkm/interop/internal/TransferToOpenGL.h>

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

namespace svtkm
{
namespace interop
{

namespace detail
{
struct TransferToOpenGL
{
  template <typename DeviceAdapterTag, typename ValueType, typename StorageTag>
  SVTKM_CONT bool operator()(DeviceAdapterTag,
                            const svtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                            BufferState& state) const
  {
    svtkm::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
    toGL.Transfer(handle);
    return true;
  }
};
}


/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// Lastly state also holds on to per backend resources that allow for efficient
/// updating to open gl.
///
/// This function keeps the buffer as the active buffer of the input type.
///
///
template <typename ValueType, class StorageTag, class DeviceAdapterTag>
SVTKM_CONT void TransferToOpenGL(const svtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                                BufferState& state,
                                DeviceAdapterTag)
{
  svtkm::interop::internal::TransferToOpenGL<ValueType, DeviceAdapterTag> toGL(state);
  toGL.Transfer(handle);
}

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible. Will use the given \p state to determine
/// what buffer handle to use, and the type to bind the buffer handle too.
/// If the type of buffer hasn't been determined the transfer will use
/// deduceAndSetBufferType to do so. Lastly state also holds on to per backend resources
/// that allow for efficient updating to open gl
///
/// This function keeps the buffer as the active buffer of the input type.
///
/// This function will throw exceptions if the transfer wasn't possible
///
template <typename ValueType, typename StorageTag>
SVTKM_CONT void TransferToOpenGL(const svtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                                BufferState& state)
{

  svtkm::cont::DeviceAdapterId devId = handle.GetDeviceAdapterId();
  bool success = svtkm::cont::TryExecuteOnDevice(devId, detail::TransferToOpenGL{}, handle, state);
  if (!success)
  {
    //Generally we are here because the devId is undefined
    //or for some reason the last executed device is now disabled
    success = svtkm::cont::TryExecute(detail::TransferToOpenGL{}, handle, state);
  }
  if (!success)
  {
    throw svtkm::cont::ErrorBadValue("Unknown device id.");
  }
}
}
}

#endif //svtk_m_interop_TransferToOpenGL_h
