//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
#define svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h

#include <svtkm/cont/cuda/ErrorCuda.h>
#include <svtkm/cont/cuda/internal/CudaAllocator.h>
#include <svtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <svtkm/cont/cuda/internal/ThrustExceptionHandler.h>
#include <svtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

#include <svtkm/cont/internal/ArrayExportMacros.h>
#include <svtkm/cont/internal/ArrayManagerExecution.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/Storage.h>

//This is in a separate header so that ArrayHandleBasicImpl can include
//the interface without getting any CUDA headers
#include <svtkm/cont/cuda/internal/ExecutionArrayInterfaceBasicCuda.h>

#include <svtkm/exec/cuda/internal/ThrustPatches.h>
SVTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_ptr.h>
SVTKM_THIRDPARTY_POST_INCLUDE

#include <limits>

// These must be placed in the svtkm::cont::internal namespace so that
// the template can be found.

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, svtkm::cont::DeviceAdapterTagCuda>
{
public:
  using ValueType = T;
  using PointerType = T*;
  using PortalType = svtkm::exec::cuda::internal::ArrayPortalFromThrust<T>;
  using PortalConstType = svtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T>;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;
  using difference_type = std::ptrdiff_t;

  SVTKM_CONT
  ArrayManagerExecution(StorageType* storage)
    : Storage(storage)
    , Begin(nullptr)
    , End(nullptr)
    , Capacity(nullptr)
  {
  }

  SVTKM_CONT
  ~ArrayManagerExecution() { this->ReleaseResources(); }

  /// Returns the size of the array.
  ///
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return static_cast<svtkm::Id>(this->End - this->Begin); }

  SVTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    try
    {
      if (updateData)
      {
        this->CopyToExecution();
      }

      return PortalConstType(this->Begin, this->End);
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  SVTKM_CONT
  PortalType PrepareForInPlace(bool updateData)
  {
    try
    {
      if (updateData)
      {
        this->CopyToExecution();
      }

      return PortalType(this->Begin, this->End);
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  SVTKM_CONT
  PortalType PrepareForOutput(svtkm::Id numberOfValues)
  {
    try
    {
      // Can we reuse the existing buffer?
      svtkm::Id curCapacity =
        this->Begin != nullptr ? static_cast<svtkm::Id>(this->Capacity - this->Begin) : 0;

      // Just mark a new end if we don't need to increase the allocation:
      if (curCapacity >= numberOfValues)
      {
        this->End = this->Begin + static_cast<difference_type>(numberOfValues);

        return PortalType(this->Begin, this->End);
      }

      const std::size_t maxNumVals = (std::numeric_limits<std::size_t>::max() / sizeof(ValueType));

      if (static_cast<std::size_t>(numberOfValues) > maxNumVals)
      {
        SVTKM_LOG_F(svtkm::cont::LogLevel::MemExec,
                   "Refusing to allocate CUDA memory; number of values (%llu) exceeds "
                   "std::size_t capacity.",
                   static_cast<svtkm::UInt64>(numberOfValues));

        std::ostringstream err;
        err << "Failed to allocate " << numberOfValues << " values on device: "
            << "Number of bytes is not representable by std::size_t.";
        throw svtkm::cont::ErrorBadAllocation(err.str());
      }

      this->ReleaseResources();

      const std::size_t bufferSize = static_cast<std::size_t>(numberOfValues) * sizeof(ValueType);

      // Attempt to allocate:
      try
      {
        this->Begin =
          static_cast<ValueType*>(svtkm::cont::cuda::internal::CudaAllocator::Allocate(bufferSize));
      }
      catch (const std::exception& error)
      {
        std::ostringstream err;
        err << "Failed to allocate " << bufferSize << " bytes on device: " << error.what();
        throw svtkm::cont::ErrorBadAllocation(err.str());
      }

      this->Capacity = this->Begin + static_cast<difference_type>(numberOfValues);
      this->End = this->Capacity;

      return PortalType(this->Begin, this->End);
    }
    catch (svtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  /// Allocates enough space in \c storage and copies the data in the
  /// device vector into it.
  ///
  SVTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    storage->Allocate(this->GetNumberOfValues());

    SVTKM_LOG_F(svtkm::cont::LogLevel::MemTransfer,
               "Copying CUDA dev --> host: %s",
               svtkm::cont::GetSizeString(this->End - this->Begin).c_str());

    try
    {
      ::thrust::copy(thrust::cuda::pointer<ValueType>(this->Begin),
                     thrust::cuda::pointer<ValueType>(this->End),
                     svtkm::cont::ArrayPortalToIteratorBegin(storage->GetPortal()));
    }
    catch (...)
    {
      svtkm::cont::cuda::internal::throwAsSVTKmException();
    }
  }

  /// Resizes the device vector.
  ///
  SVTKM_CONT void Shrink(svtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    SVTKM_ASSERT(this->Begin != nullptr && this->Begin + numberOfValues <= this->End);

    this->End = this->Begin + static_cast<difference_type>(numberOfValues);
  }

  /// Frees all memory.
  ///
  SVTKM_CONT void ReleaseResources()
  {
    if (this->Begin != nullptr)
    {
      svtkm::cont::cuda::internal::CudaAllocator::Free(this->Begin);
      this->Begin = nullptr;
      this->End = nullptr;
      this->Capacity = nullptr;
    }
  }

private:
  ArrayManagerExecution(ArrayManagerExecution&) = delete;
  void operator=(ArrayManagerExecution&) = delete;

  StorageType* Storage;

  PointerType Begin;
  PointerType End;
  PointerType Capacity;

  SVTKM_CONT
  void CopyToExecution()
  {
    try
    {
      this->PrepareForOutput(this->Storage->GetNumberOfValues());

      SVTKM_LOG_F(svtkm::cont::LogLevel::MemTransfer,
                 "Copying host --> CUDA dev: %s.",
                 svtkm::cont::GetSizeString(this->End - this->Begin).c_str());

      ::thrust::copy(svtkm::cont::ArrayPortalToIteratorBegin(this->Storage->GetPortalConst()),
                     svtkm::cont::ArrayPortalToIteratorEnd(this->Storage->GetPortalConst()),
                     thrust::cuda::pointer<ValueType>(this->Begin));
    }
    catch (...)
    {
      svtkm::cont::cuda::internal::throwAsSVTKmException();
    }
  }
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagCuda>
{
  using ValueType = T;
  using PortalType = svtkm::exec::cuda::internal::ArrayPortalFromThrust<ValueType>;
  using PortalConstType = svtkm::exec::cuda::internal::ConstArrayPortalFromThrust<ValueType>;

  SVTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end)
  {
    return PortalType(start, end);
  }

  SVTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end)
  {
    return PortalConstType(start, end);
  }
};

} // namespace internal

#ifndef svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
SVTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagCuda)
#endif // !svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
}
} // namespace svtkm::cont

#endif //svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
