//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/cuda/internal/testing/Testing.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/cuda/ErrorCuda.h>

#include <svtkm/cont/cuda/internal/CudaAllocator.h>
#include <svtkm/cont/cuda/internal/testing/Testing.h>

#include <cuda_runtime.h>

using svtkm::cont::cuda::internal::CudaAllocator;

namespace
{

template <typename ValueType>
ValueType* AllocateManagedPointer(svtkm::Id numValues)
{
  void* result;
  SVTKM_CUDA_CALL(cudaMallocManaged(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                   "Allocated pointer is not a device pointer.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(result), "Allocated pointer is not managed.");
  return static_cast<ValueType*>(result);
}

template <typename ValueType>
ValueType* AllocateDevicePointer(svtkm::Id numValues)
{
  void* result;
  SVTKM_CUDA_CALL(cudaMalloc(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                   "Allocated pointer is not a device pointer.");
  SVTKM_TEST_ASSERT(!CudaAllocator::IsManagedPointer(result), "Allocated pointer is managed.");
  return static_cast<ValueType*>(result);
}

template <typename ValueType>
svtkm::cont::ArrayHandle<ValueType> CreateArrayHandle(svtkm::Id numValues, bool managed)
{
  ValueType* ptr = managed ? AllocateManagedPointer<ValueType>(numValues)
                           : AllocateDevicePointer<ValueType>(numValues);
  return svtkm::cont::make_ArrayHandle(ptr, numValues);
}

template <typename ValueType>
void TestPrepareForInput(bool managed)
{
  svtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  handle.PrepareForInput(svtkm::cont::DeviceAdapterTagCuda());

  auto lock = handle.Internals->GetLock();
  void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
  void* execArray = handle.Internals->Internals->GetExecutionArray(lock);
  SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after PrepareForInput.");
  SVTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInput.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(execArray),
                   "PrepareForInput execution array not device pointer.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                   "PrepareForInput control array not device pointer.");
  if (managed)
  {
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray),
                     "PrepareForInput execution array unmanaged.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray),
                     "PrepareForInput control array unmanaged.");
  }
  SVTKM_TEST_ASSERT(execArray == contArray, "PrepareForInput managed arrays not shared.");
}

template <typename ValueType>
void TestPrepareForInPlace(bool managed)
{
  svtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  handle.PrepareForInPlace(svtkm::cont::DeviceAdapterTagCuda());

  auto lock = handle.Internals->GetLock();
  void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
  void* execArray = handle.Internals->Internals->GetExecutionArray(lock);
  SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after PrepareForInPlace.");
  SVTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInPlace.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(execArray),
                   "PrepareForInPlace execution array not device pointer.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                   "PrepareForInPlace control array not device pointer.");
  if (managed)
  {
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray),
                     "PrepareForInPlace execution array unmanaged.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray),
                     "PrepareForInPlace control array unmanaged.");
  }
  SVTKM_TEST_ASSERT(execArray == contArray, "PrepareForInPlace managed arrays not shared.");
}

template <typename ValueType>
void TestPrepareForOutput(bool managed)
{
  // Should reuse a managed control pointer if buffer is large enough.
  svtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  handle.PrepareForOutput(32, svtkm::cont::DeviceAdapterTagCuda());

  auto lock = handle.Internals->GetLock();
  void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
  void* execArray = handle.Internals->Internals->GetExecutionArray(lock);
  SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after PrepareForOutput.");
  SVTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForOutput.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(execArray),
                   "PrepareForOutput execution array not device pointer.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                   "PrepareForOutput control array not device pointer.");
  if (managed)
  {
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray),
                     "PrepareForOutput execution array unmanaged.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray),
                     "PrepareForOutput control array unmanaged.");
  }
  SVTKM_TEST_ASSERT(execArray == contArray, "PrepareForOutput managed arrays not shared.");
}

template <typename ValueType>
void TestReleaseResourcesExecution(bool managed)
{
  svtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  handle.PrepareForInput(svtkm::cont::DeviceAdapterTagCuda());

  void* origArray;
  {
    auto lock = handle.Internals->GetLock();
    origArray = handle.Internals->Internals->GetExecutionArray(lock);
  }

  handle.ReleaseResourcesExecution();

  auto lock = handle.Internals->GetLock();
  void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
  void* execArray = handle.Internals->Internals->GetExecutionArray(lock);

  SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after ReleaseResourcesExecution.");
  SVTKM_TEST_ASSERT(execArray == nullptr,
                   "Execution array not cleared after ReleaseResourcesExecution.");
  SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                   "ReleaseResourcesExecution control array not device pointer.");
  if (managed)
  {
    SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray),
                     "ReleaseResourcesExecution control array unmanaged.");
  }
  SVTKM_TEST_ASSERT(origArray == contArray,
                   "Control array changed after ReleaseResourcesExecution.");
}

template <typename ValueType>
void TestRoundTrip(bool managed)
{
  svtkm::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  handle.PrepareForOutput(32, svtkm::cont::DeviceAdapterTagCuda());

  void* origContArray;
  {
    auto lock = handle.Internals->GetLock();
    origContArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
  }
  {
    auto lock = handle.Internals->GetLock();
    void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
    void* execArray = handle.Internals->Internals->GetExecutionArray(lock);
    SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after PrepareForOutput.");
    SVTKM_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForOutput.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(execArray),
                     "PrepareForOutput execution array not device pointer.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                     "PrepareForOutput control array not device pointer.");
    if (managed)
    {
      SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray),
                       "PrepareForOutput execution array unmanaged.");
      SVTKM_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray),
                       "PrepareForOutput control array unmanaged.");
    }
    SVTKM_TEST_ASSERT(execArray == contArray, "PrepareForOutput managed arrays not shared.");
  }

  try
  {
    handle.GetPortalControl();
  }
  catch (svtkm::cont::ErrorBadValue&)
  {
    if (managed)
    {
      throw; // Exception is unexpected
    }

    // If !managed, this exception is intentional to indicate that the control
    // array is a non-managed device pointer, and thus unaccessible from the
    // control environment. Return because we've already validated correct
    // behavior by catching this exception.
    return;
  }

  if (!managed)
  {
    SVTKM_TEST_FAIL("Expected exception not thrown.");
  }

  {
    auto lock = handle.Internals->GetLock();
    void* contArray = handle.Internals->Internals->GetControlArray(lock)->GetBasePointer();
    void* execArray = handle.Internals->Internals->GetExecutionArray(lock);
    SVTKM_TEST_ASSERT(contArray != nullptr, "No control array after GetPortalConst.");
    SVTKM_TEST_ASSERT(execArray == nullptr, "Execution array not cleared after GetPortalConst.");
    SVTKM_TEST_ASSERT(CudaAllocator::IsDevicePointer(contArray),
                     "GetPortalConst control array not device pointer.");
    SVTKM_TEST_ASSERT(origContArray == contArray, "GetPortalConst changed control array.");
  }
}

template <typename ValueType>
void DoTests()
{
  TestPrepareForInput<ValueType>(false);
  TestPrepareForInPlace<ValueType>(false);
  TestPrepareForOutput<ValueType>(false);
  TestReleaseResourcesExecution<ValueType>(false);
  TestRoundTrip<ValueType>(false);


  // If this device does not support managed memory, skip the managed tests.
  if (!CudaAllocator::UsingManagedMemory())
  {
    std::cerr << "Skipping some tests -- device does not support managed memory.\n";
  }
  else
  {
    TestPrepareForInput<ValueType>(true);
    TestPrepareForInPlace<ValueType>(true);
    TestPrepareForOutput<ValueType>(true);
    TestReleaseResourcesExecution<ValueType>(true);
    TestRoundTrip<ValueType>(true);
  }
}

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    DoTests<ValueType>();
  }
};

void Launch()
{
  using Types = svtkm::List<svtkm::UInt8,
                           svtkm::Vec<svtkm::UInt8, 3>,
                           svtkm::Float32,
                           svtkm::Vec<svtkm::Float32, 4>,
                           svtkm::Float64,
                           svtkm::Vec<svtkm::Float64, 4>>;
  svtkm::testing::Testing::TryTypes(ArgToTemplateType(), Types());
}

} // end anon namespace

int UnitTestCudaShareUserProvidedManagedMemory(int argc, char* argv[])
{
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(svtkm::cont::DeviceAdapterTagCuda{});
  int ret = svtkm::cont::testing::Testing::Run(Launch, argc, argv);
  return svtkm::cont::cuda::internal::Testing::CheckCudaBeforeExit(ret);
}
