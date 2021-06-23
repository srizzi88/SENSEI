//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h
#define svtk_m_cont_cuda_internal_DeviceAdapterTimerImplementationCuda_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>

#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#include <cuda.h>

namespace svtkm
{
namespace cont
{

///
/// Specialization of DeviceAdapterTimerImplementation for CUDA
/// CUDA contains its own high resolution timer that are able
/// to track how long it takes to execute async kernels.
/// If we simply measured time on the CPU it would incorrectly
/// just capture how long it takes to launch a kernel.
template <>
class SVTKM_CONT_EXPORT DeviceAdapterTimerImplementation<svtkm::cont::DeviceAdapterTagCuda>
{
public:
  SVTKM_CONT DeviceAdapterTimerImplementation();

  SVTKM_CONT ~DeviceAdapterTimerImplementation();

  SVTKM_CONT void Reset();

  SVTKM_CONT void Start();

  SVTKM_CONT void Stop();

  SVTKM_CONT bool Started() const;

  SVTKM_CONT bool Stopped() const;

  SVTKM_CONT bool Ready() const;

  SVTKM_CONT svtkm::Float64 GetElapsedTime() const;

private:
  // Copying CUDA events is problematic.
  DeviceAdapterTimerImplementation(
    const DeviceAdapterTimerImplementation<svtkm::cont::DeviceAdapterTagCuda>&) = delete;
  void operator=(const DeviceAdapterTimerImplementation<svtkm::cont::DeviceAdapterTagCuda>&) =
    delete;

  bool StartReady;
  bool StopReady;
  cudaEvent_t StartEvent;
  cudaEvent_t StopEvent;
};
}
} // namespace svtkm::cont


#endif
