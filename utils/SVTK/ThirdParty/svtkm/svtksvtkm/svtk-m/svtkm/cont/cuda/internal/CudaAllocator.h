//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_cuda_internal_CudaAllocator_h
#define svtk_m_cont_cuda_internal_CudaAllocator_h

#include <svtkm/cont/svtkm_cont_export.h>
#include <svtkm/internal/ExportMacros.h>

#include <cstddef>

namespace svtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// Collection of cuda-specific memory management operations.
struct SVTKM_CONT_EXPORT CudaAllocator
{
  /// Returns true if all detected CUDA devices support pageable managed memory
  /// that can be accessed concurrently by the CPU and GPUs.
  static SVTKM_CONT bool UsingManagedMemory();

  /// Force CUDA allocations to occur with unmanaged memory (aka cudaMalloc).
  static SVTKM_CONT void ForceManagedMemoryOff();

  /// Force CUDA allocations to occur with pageable managed memory.
  /// If the current hardware doesn't support pageable managed memory
  /// SVTK-m will ignore the request and continue to use unmanaged memory (aka cudaMalloc).
  static SVTKM_CONT void ForceManagedMemoryOn();

  /// Returns true if the pointer is accessible from a CUDA device.
  static SVTKM_CONT bool IsDevicePointer(const void* ptr);

  /// Returns true if the pointer is a CUDA pointer allocated with
  /// cudaMallocManaged.
  static SVTKM_CONT bool IsManagedPointer(const void* ptr);

  /// Will allocate memory that could be managed or unmanaged
  static SVTKM_CONT void* Allocate(std::size_t numBytes);

  /// Explicitly allocate unmanaged memory even when the device supports
  /// managed memory
  static SVTKM_CONT void* AllocateUnManaged(std::size_t numBytes);

  /// Explicitly deallocate memory immediately.
  static SVTKM_CONT void Free(void* ptr);

  /// \brief Defer deallocation of some memory
  ///
  /// Keeps a pool of pointers to free until such a time of as we have
  /// meet a threshold in total memory or number of pointers.
  /// Currently the threshold to free all the pointers is 16MB
  ///
  /// The reason for using this is that cudaFree causes a cudaSync call
  /// to occur across all cuda devices and streams. This causes lots of stalls
  /// when we are constructing small objects like virtuals and function pointers.
  static SVTKM_CONT void FreeDeferred(void* ptr, std::size_t numBytes);

  static SVTKM_CONT void PrepareForControl(const void* ptr, std::size_t numBytes);

  static SVTKM_CONT void PrepareForInput(const void* ptr, std::size_t numBytes);
  static SVTKM_CONT void PrepareForOutput(const void* ptr, std::size_t numBytes);
  static SVTKM_CONT void PrepareForInPlace(const void* ptr, std::size_t numBytes);

private:
  static SVTKM_CONT void Initialize();
};
}
}
}
} // end namespace svtkm::cont::cuda::internal

#endif // svtk_m_cont_cuda_internal_CudaAllocator_h
