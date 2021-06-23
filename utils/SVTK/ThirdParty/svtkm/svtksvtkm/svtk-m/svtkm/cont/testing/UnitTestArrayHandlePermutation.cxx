//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandlePermutation.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/StorageBasic.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

struct DoubleIndexFunctor
{
  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id index) const { return 2 * index; }
};

using DoubleIndexArrayType = svtkm::cont::ArrayHandleImplicit<DoubleIndexFunctor>;

template <typename PermutedPortalType>
struct CheckPermutationFunctor : svtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    T value = this->PermutedPortal.Get(index);

    svtkm::Id permutedIndex = 2 * index;
    T expectedValue = TestValue(permutedIndex, T());

    if (!test_equal(value, expectedValue))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename PermutedArrayHandleType, typename Device>
SVTKM_CONT CheckPermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::PortalConst>
make_CheckPermutationFunctor(const PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  CheckPermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForInput(Device());
  return functor;
}

template <typename PermutedPortalType>
struct InPlacePermutationFunctor : svtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    T value = this->PermutedPortal.Get(index);

    value = value + T(1000);

    this->PermutedPortal.Set(index, value);
  }
};

template <typename PermutedArrayHandleType, typename Device>
SVTKM_CONT InPlacePermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal>
make_InPlacePermutationFunctor(PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal;
  InPlacePermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForInPlace(Device());
  return functor;
}

template <typename PortalType>
SVTKM_CONT void CheckInPlaceResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (svtkm::Id permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      T expectedValue = TestValue(permutedIndex, T()) + T(1000);
      T retrievedValue = portal.Get(permutedIndex);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue), "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "Permuted array modified value it should not have.");
    }
  }
}

template <typename PermutedPortalType>
struct OutputPermutationFunctor : svtkm::exec::FunctorBase
{
  PermutedPortalType PermutedPortal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using T = typename PermutedPortalType::ValueType;
    this->PermutedPortal.Set(index, TestValue(static_cast<svtkm::Id>(index), T()));
  }
};

template <typename PermutedArrayHandleType, typename Device>
SVTKM_CONT OutputPermutationFunctor<
  typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal>
make_OutputPermutationFunctor(PermutedArrayHandleType& permutedArray, Device)
{
  using PermutedPortalType =
    typename PermutedArrayHandleType::template ExecutionTypes<Device>::Portal;
  OutputPermutationFunctor<PermutedPortalType> functor;
  functor.PermutedPortal = permutedArray.PrepareForOutput(ARRAY_SIZE, Device());
  return functor;
}

template <typename PortalType>
SVTKM_CONT void CheckOutputResult(PortalType portal)
{
  using T = typename PortalType::ValueType;
  for (svtkm::IdComponent permutedIndex = 0; permutedIndex < 2 * ARRAY_SIZE; permutedIndex++)
  {
    if (permutedIndex % 2 == 0)
    {
      // This index was part of the permuted array; has a value changed
      svtkm::Id originalIndex = permutedIndex / 2;
      T expectedValue = TestValue(originalIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue), "Permuted set unexpected value.");
    }
    else
    {
      // This index was not part of the permuted array; has original value
      T expectedValue = TestValue(permutedIndex, T());
      T retrievedValue = portal.Get(permutedIndex);
      SVTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "Permuted array modified value it should not have.");
    }
  }
}

template <typename ValueType>
struct PermutationTests
{
  using IndexArrayType = svtkm::cont::ArrayHandleImplicit<DoubleIndexFunctor>;
  using ValueArrayType = svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTagBasic>;
  using PermutationArrayType = svtkm::cont::ArrayHandlePermutation<IndexArrayType, ValueArrayType>;

  using Device = svtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<Device>;

  ValueArrayType MakeValueArray() const
  {
    // Allocate a buffer and set initial values
    std::vector<ValueType> buffer(2 * ARRAY_SIZE);
    for (svtkm::IdComponent index = 0; index < 2 * ARRAY_SIZE; index++)
    {
      svtkm::UInt32 i = static_cast<svtkm::UInt32>(index);
      buffer[i] = TestValue(index, ValueType());
    }

    // Create an ArrayHandle from the buffer
    ValueArrayType array = svtkm::cont::make_ArrayHandle(buffer);

    // Copy the array so that the data is not destroyed when we return from
    // this method.
    ValueArrayType arrayCopy;
    Algorithm::Copy(array, arrayCopy);

    return arrayCopy;
  }

  void operator()() const
  {
    std::cout << "Create ArrayHandlePermutation" << std::endl;
    IndexArrayType indexArray(DoubleIndexFunctor(), ARRAY_SIZE);

    ValueArrayType valueArray = this->MakeValueArray();

    PermutationArrayType permutationArray(indexArray, valueArray);

    SVTKM_TEST_ASSERT(permutationArray.GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation array wrong size.");
    SVTKM_TEST_ASSERT(permutationArray.GetPortalControl().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");
    SVTKM_TEST_ASSERT(permutationArray.GetPortalConstControl().GetNumberOfValues() == ARRAY_SIZE,
                     "Permutation portal wrong size.");

    std::cout << "Test initial values in execution environment" << std::endl;
    Algorithm::Schedule(make_CheckPermutationFunctor(permutationArray, Device()), ARRAY_SIZE);

    std::cout << "Try in place operation" << std::endl;
    Algorithm::Schedule(make_InPlacePermutationFunctor(permutationArray, Device()), ARRAY_SIZE);
    CheckInPlaceResult(valueArray.GetPortalControl());
    CheckInPlaceResult(valueArray.GetPortalConstControl());

    std::cout << "Try output operation" << std::endl;
    Algorithm::Schedule(make_OutputPermutationFunctor(permutationArray, Device()), ARRAY_SIZE);
    CheckOutputResult(valueArray.GetPortalConstControl());
    CheckOutputResult(valueArray.GetPortalControl());
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    PermutationTests<InputType>()();
  }
};

void TestArrayHandlePermutation()
{
  svtkm::testing::Testing::TryTypes(TryInputType(), svtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandlePermutation(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayHandlePermutation, argc, argv);
}
