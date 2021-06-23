//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h
#define svtk_m_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>

#include <svtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

namespace svtkm
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;


/// \brief Class providing a CUDA runtime support detector.
///
/// The class provide the actual implementation used by
/// svtkm::cont::RuntimeDeviceInformation for the CUDA backend.
///
/// We will verify at runtime that the machine has at least one CUDA
/// capable device, and said device is from the 'fermi' (SM_20) generation
/// or newer.
///
template <>
class SVTKM_CONT_EXPORT DeviceAdapterRuntimeDetector<svtkm::cont::DeviceAdapterTagCuda>
{
public:
  SVTKM_CONT DeviceAdapterRuntimeDetector();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  /// Only returns true if we have at-least one CUDA capable device of SM_20 or
  /// greater ( fermi ).
  ///
  SVTKM_CONT bool Exists() const;

private:
  svtkm::Int32 NumberOfDevices;
  svtkm::Int32 HighestArchSupported;
};
}
} // namespace svtkm::cont


#endif
