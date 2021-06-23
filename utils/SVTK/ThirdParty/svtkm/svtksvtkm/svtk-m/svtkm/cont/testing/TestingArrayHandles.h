//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingArrayHandles_h
#define svtk_m_cont_testing_TestingArrayHandles_h

#include <svtkm/TypeTraits.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/ArrayHandleExtractComponent.h>
#include <svtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace svtkm
{
namespace cont
{
namespace testing
{

namespace array_handle_testing
{
template <class IteratorType, typename T>
void CheckValues(IteratorType begin, IteratorType end, T)
{

  svtkm::Id index = 0;
  for (IteratorType iter = begin; iter != end; iter++)
  {
    T expectedValue = TestValue(index, T());
    if (!test_equal(*iter, expectedValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue << ", Found: " << *iter << std::endl;
      SVTKM_TEST_FAIL(message.str().c_str());
    }

    index++;
  }
}

template <typename T>
void CheckArray(const svtkm::cont::ArrayHandle<T>& handle)
{
  CheckPortal(handle.GetPortalConstControl());
}
}

// Use to get an arbitrarily different valuetype than T:
template <typename T>
struct OtherType
{
  using Type = svtkm::Int32;
};
template <>
struct OtherType<svtkm::Int32>
{
  using Type = svtkm::UInt8;
};

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with the given DeviceAdapter
///
template <class DeviceAdapterTag>
struct TestingArrayHandles
{

  struct PassThrough : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = _2(_1);

    template <class ValueType>
    SVTKM_EXEC ValueType operator()(const ValueType& inValue) const
    {
      return inValue;
    }
  };

  template <typename T, typename ExecutionPortalType>
  struct AssignTestValue : public svtkm::exec::FunctorBase
  {
    ExecutionPortalType Portal;
    SVTKM_CONT
    AssignTestValue(ExecutionPortalType p)
      : Portal(p)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id index) const { this->Portal.Set(index, TestValue(index, T())); }
  };

  template <typename T, typename ExecutionPortalType>
  struct InplaceFunctor : public svtkm::exec::FunctorBase
  {
    ExecutionPortalType Portal;
    SVTKM_CONT
    InplaceFunctor(const ExecutionPortalType& p)
      : Portal(p)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id index) const
    {
      this->Portal.Set(index, T(this->Portal.Get(index) + T(1)));
    }
  };

private:
  static constexpr svtkm::Id ARRAY_SIZE = 100;

  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

  using DispatcherPassThrough = svtkm::worklet::DispatcherMapField<PassThrough>;
  struct VerifyEmptyArrays
  {
    template <typename T>
    SVTKM_CONT void operator()(T) const
    {
      std::cout << "Try operations on empty arrays." << std::endl;
      // After each operation, reinitialize array in case something gets
      // allocated.
      svtkm::cont::ArrayHandle<T> arrayHandle = svtkm::cont::ArrayHandle<T>();
      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                       "Uninitialized array does not report zero values.");
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      SVTKM_TEST_ASSERT(arrayHandle.GetPortalConstControl().GetNumberOfValues() == 0,
                       "Uninitialized array does not give portal with zero values.");
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      arrayHandle.Shrink(0);
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      arrayHandle.ReleaseResourcesExecution();
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      arrayHandle.ReleaseResources();
      arrayHandle = svtkm::cont::make_ArrayHandle(std::vector<T>());
      arrayHandle.PrepareForInput(DeviceAdapterTag());
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      arrayHandle.PrepareForInPlace(DeviceAdapterTag());
      arrayHandle = svtkm::cont::ArrayHandle<T>();
      arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());
    }
  };

  struct VerifyUserOwnedMemory
  {
    template <typename T>
    SVTKM_CONT void operator()(T) const
    {
      std::vector<T> buffer(ARRAY_SIZE);
      for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
      }

      svtkm::cont::ArrayHandle<T> arrayHandle = svtkm::cont::make_ArrayHandle(buffer);

      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                       "ArrayHandle has wrong number of entries.");

      std::cout << "Check array with user provided memory." << std::endl;
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
          executionPortal;
        executionPortal = arrayHandle.PrepareForInput(DeviceAdapterTag());

        //use a worklet to verify the input transfer worked properly
        svtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal
          executionPortal;
        executionPortal = arrayHandle.PrepareForInPlace(DeviceAdapterTag());

        //use a worklet to verify the inplace transfer worked properly
        svtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal
          executionPortal;
        executionPortal = arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
      }

      { //as output with a length larger than the memory provided by the user
        //this should fail
        bool gotException = false;
        try
        {
          //you should not be able to allocate a size larger than the
          //user provided and get the results
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag());
          arrayHandle.GetPortalControl();
        }
        catch (svtkm::cont::Error&)
        {
          gotException = true;
        }
        SVTKM_TEST_ASSERT(gotException,
                         "PrepareForOutput should fail when asked to "
                         "re-allocate user provided memory.");
      }
    }
  };


  struct VerifyUserTransferredMemory
  {
    template <typename T>
    SVTKM_CONT void operator()(T) const
    {
      T* buffer = new T[ARRAY_SIZE];
      for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
      }

      auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };
      svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic> storage(
        buffer, ARRAY_SIZE, user_free_function);
      svtkm::cont::ArrayHandle<T> arrayHandle(std::move(storage));

      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                       "ArrayHandle has wrong number of entries.");

      std::cout << "Check array with user transferred memory." << std::endl;
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Check out execution array behavior." << std::endl;
      { //as input
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::PortalConst
          executionPortal;
        executionPortal = arrayHandle.PrepareForInput(DeviceAdapterTag());

        //use a worklet to verify the input transfer worked properly
        svtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out inplace." << std::endl;
      { //as inplace
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal
          executionPortal;
        executionPortal = arrayHandle.PrepareForInPlace(DeviceAdapterTag());

        //use a worklet to verify the inplace transfer worked properly
        svtkm::cont::ArrayHandle<T> result;
        DispatcherPassThrough().Invoke(arrayHandle, result);
        array_handle_testing::CheckArray(result);
      }

      std::cout << "Check out output." << std::endl;
      { //as output with same length as user provided. This should work
        //as no new memory needs to be allocated
        typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal
          executionPortal;
        executionPortal = arrayHandle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag());

        //we can't verify output contents as those aren't fetched, we
        //can just make sure the allocation didn't throw an exception
      }

      { //as the memory ownership has been transferred to SVTK-m this should
        //allow SVTK-m to free the memory and allocate a new block
        bool gotException = false;
        try
        {
          //you should not be able to allocate a size larger than the
          //user provided and get the results
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag());
          arrayHandle.GetPortalControl();
        }
        catch (svtkm::cont::Error&)
        {
          gotException = true;
        }
        SVTKM_TEST_ASSERT(!gotException,
                         "PrepareForOutput shouldn't fail when asked to "
                         "re-allocate user transferred memory.");
      }
    }
  };

  struct VerifySVTKMAllocatedHandle
  {
    template <typename T>
    SVTKM_CONT void operator()(T) const
    {
      svtkm::cont::ArrayHandle<T> arrayHandle;

      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                       "ArrayHandle has wrong number of entries.");
      {
        using ExecutionPortalType =
          typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal;
        ExecutionPortalType executionPortal =
          arrayHandle.PrepareForOutput(ARRAY_SIZE * 2, DeviceAdapterTag());

        //we drop down to manually scheduling so that we don't need
        //need to bring in array handle counting
        AssignTestValue<T, ExecutionPortalType> functor(executionPortal);
        Algorithm::Schedule(functor, ARRAY_SIZE * 2);
      }

      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Array not allocated correctly.");
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Try shrinking the array." << std::endl;
      arrayHandle.Shrink(ARRAY_SIZE);
      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                       "Array size did not shrink correctly.");
      array_handle_testing::CheckArray(arrayHandle);

      std::cout << "Try reallocating array." << std::endl;
      arrayHandle.Allocate(ARRAY_SIZE * 2);
      SVTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Array size did not allocate correctly.");
      // No point in checking values. This method can invalidate them.

      std::cout << "Try in place operation." << std::endl;
      {
        using ExecutionPortalType =
          typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapterTag>::Portal;
        ExecutionPortalType executionPortal = arrayHandle.PrepareForInPlace(DeviceAdapterTag());

        //in place can't be done through the dispatcher
        //instead we have to drop down to manually scheduling
        InplaceFunctor<T, ExecutionPortalType> functor(executionPortal);
        Algorithm::Schedule(functor, ARRAY_SIZE * 2);
      }
      typename svtkm::cont::ArrayHandle<T>::PortalConstControl controlPortal =
        arrayHandle.GetPortalConstControl();
      for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
      {
        SVTKM_TEST_ASSERT(test_equal(controlPortal.Get(index), TestValue(index, T()) + T(1)),
                         "Did not get result from in place operation.");
      }

      SVTKM_TEST_ASSERT(arrayHandle == arrayHandle, "Array handle does not equal itself.");
      SVTKM_TEST_ASSERT(arrayHandle != svtkm::cont::ArrayHandle<T>(),
                       "Array handle equals different array.");
    }
  };

  struct VerifyEqualityOperators
  {
    template <typename T>
    SVTKM_CONT void operator()(T) const
    {
      std::cout << "Verify that shallow copied array handles compare equal:\n";
      {
        svtkm::cont::ArrayHandle<T> a1;
        svtkm::cont::ArrayHandle<T> a2 = a1; // shallow copy
        svtkm::cont::ArrayHandle<T> a3;
        SVTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
        SVTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
        SVTKM_TEST_ASSERT(a1 != a3, "Distinct arrays compared equal.");
        SVTKM_TEST_ASSERT(!(a1 == a3), "Distinct arrays compared equal.");

        // Operations on a1 shouldn't affect equality
        a1.Allocate(200);
        SVTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
        SVTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

        a1.GetPortalConstControl();
        SVTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
        SVTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

        a1.PrepareForInPlace(DeviceAdapterTag());
        SVTKM_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
        SVTKM_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
      }

      std::cout << "Verify that handles with different storage types are not equal.\n";
      {
        svtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageTagBasic> tmp;
        auto a2 = svtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

        SVTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage type compared equal.");
        SVTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different storage type compared equal.");
      }

      std::cout << "Verify that handles with different value types are not equal.\n";
      {
        svtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        svtkm::cont::ArrayHandle<typename OtherType<T>::Type, StorageTagBasic> a2;

        SVTKM_TEST_ASSERT(a1 != a2, "Arrays with different value type compared equal.");
        SVTKM_TEST_ASSERT(!(a1 == a2), "Arrays with different value type compared equal.");
      }

      std::cout << "Verify that handles with different storage and value types are not equal.\n";
      {
        svtkm::cont::ArrayHandle<T, StorageTagBasic> a1;
        svtkm::cont::ArrayHandle<svtkm::Vec<typename OtherType<T>::Type, 3>, StorageTagBasic> tmp;
        auto a2 = svtkm::cont::make_ArrayHandleExtractComponent(tmp, 1);

        SVTKM_TEST_ASSERT(a1 != a2, "Arrays with different storage and value type compared equal.");
        SVTKM_TEST_ASSERT(!(a1 == a2),
                         "Arrays with different storage and value type compared equal.");
      }
    }
  };

  struct TryArrayHandleType
  {
    void operator()() const
    {
      svtkm::testing::Testing::TryTypes(VerifyEmptyArrays());
      svtkm::testing::Testing::TryTypes(VerifyUserOwnedMemory());
      svtkm::testing::Testing::TryTypes(VerifyUserTransferredMemory());
      svtkm::testing::Testing::TryTypes(VerifySVTKMAllocatedHandle());
      svtkm::testing::Testing::TryTypes(VerifyEqualityOperators());
    }
  };

public:
  static SVTKM_CONT int Run(int argc, char* argv[])
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return svtkm::cont::testing::Testing::Run(TryArrayHandleType(), argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif //svtk_m_cont_testing_TestingArrayHandles_h
