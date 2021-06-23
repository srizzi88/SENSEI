//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherReduceByKey.h>
#include <svtkm/worklet/WorkletReduceByKey.h>

#include <svtkm/worklet/Keys.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/DeviceAdapterTag.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

#define STRINGIFY(x) STRINGIFY_IMPL(x)
#define STRINGIFY_IMPL(x) #x

#define TEST_ASSERT_WORKLET(condition)                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      this->RaiseError("Test assert failed: " #condition "\n" __FILE__ ":" STRINGIFY(__LINE__));   \
      return;                                                                                      \
    }                                                                                              \
  } while (false)

#define ARRAY_SIZE 1033
#define GROUP_SIZE 10
#define NUM_UNIQUE (svtkm::Id)(ARRAY_SIZE / GROUP_SIZE)

struct CheckKeyValuesWorklet : svtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn keyMirror,
                                ValuesIn indexValues,
                                ValuesInOut valuesToModify,
                                ValuesOut writeKey);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex, ValueCount);
  using InputDomain = _1;

  template <typename T,
            typename KeyMirrorVecType,
            typename IndexValuesVecType,
            typename ValuesToModifyVecType,
            typename WriteKeysVecType>
  SVTKM_EXEC void operator()(const T& key,
                            const KeyMirrorVecType& keyMirror,
                            const IndexValuesVecType& valueIndices,
                            ValuesToModifyVecType& valuesToModify,
                            WriteKeysVecType& writeKey,
                            svtkm::Id workIndex,
                            svtkm::IdComponent numValues) const
  {
    // These tests only work if keys are in sorted order, which is how we group
    // them.

    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    TEST_ASSERT_WORKLET(numValues >= GROUP_SIZE);
    TEST_ASSERT_WORKLET(keyMirror.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valueIndices.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(valuesToModify.GetNumberOfComponents() == numValues);
    TEST_ASSERT_WORKLET(writeKey.GetNumberOfComponents() == numValues);

    for (svtkm::IdComponent iComponent = 0; iComponent < numValues; iComponent++)
    {
      TEST_ASSERT_WORKLET(test_equal(keyMirror[iComponent], key));
      TEST_ASSERT_WORKLET(valueIndices[iComponent] % NUM_UNIQUE == workIndex);

      T value = valuesToModify[iComponent];
      valuesToModify[iComponent] = static_cast<T>(key + value);

      writeKey[iComponent] = key;
    }
  }
};

struct CheckReducedValuesWorklet : svtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn,
                                ReducedValuesOut extractKeys,
                                ReducedValuesIn indexReference,
                                ReducedValuesInOut copyKeyPair);
  using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

  template <typename T>
  SVTKM_EXEC void operator()(const T& key,
                            T& reducedValueOut,
                            svtkm::Id indexReference,
                            svtkm::Pair<T, T>& copyKeyPair,
                            svtkm::Id workIndex) const
  {
    // This check only work if keys are in sorted order, which is how we group
    // them.
    TEST_ASSERT_WORKLET(key == TestValue(workIndex, T()));

    reducedValueOut = key;

    TEST_ASSERT_WORKLET(indexReference == workIndex);

    TEST_ASSERT_WORKLET(copyKeyPair.first == key);
    copyKeyPair.second = key;
  }
};

template <typename KeyType>
void TryKeyType(KeyType)
{
  KeyType keyBuffer[ARRAY_SIZE];
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }

  svtkm::cont::ArrayHandle<KeyType> keyArray = svtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  svtkm::cont::ArrayHandle<KeyType> sortedKeys;
  svtkm::cont::ArrayCopy(keyArray, sortedKeys);

  svtkm::worklet::Keys<KeyType> keys(sortedKeys);

  svtkm::cont::ArrayHandle<KeyType> valuesToModify;
  valuesToModify.Allocate(ARRAY_SIZE);
  SetPortal(valuesToModify.GetPortalControl());

  svtkm::cont::ArrayHandle<KeyType> writeKey;

  svtkm::worklet::DispatcherReduceByKey<CheckKeyValuesWorklet> dispatcherCheckKeyValues;
  dispatcherCheckKeyValues.Invoke(
    keys, keyArray, svtkm::cont::ArrayHandleIndex(ARRAY_SIZE), valuesToModify, writeKey);

  SVTKM_TEST_ASSERT(valuesToModify.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  SVTKM_TEST_ASSERT(writeKey.GetNumberOfValues() == ARRAY_SIZE, "Bad array size.");
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    KeyType key = TestValue(index % NUM_UNIQUE, KeyType());
    KeyType value = TestValue(index, KeyType());

    SVTKM_TEST_ASSERT(test_equal(static_cast<KeyType>(key + value),
                                valuesToModify.GetPortalConstControl().Get(index)),
                     "Bad in/out value.");

    SVTKM_TEST_ASSERT(test_equal(key, writeKey.GetPortalConstControl().Get(index)),
                     "Bad out value.");
  }

  svtkm::cont::ArrayHandle<KeyType> keyPairIn;
  keyPairIn.Allocate(NUM_UNIQUE);
  SetPortal(keyPairIn.GetPortalControl());

  svtkm::cont::ArrayHandle<KeyType> keyPairOut;
  keyPairOut.Allocate(NUM_UNIQUE);

  svtkm::worklet::DispatcherReduceByKey<CheckReducedValuesWorklet> dispatcherCheckReducedValues;
  dispatcherCheckReducedValues.Invoke(keys,
                                      writeKey,
                                      svtkm::cont::ArrayHandleIndex(NUM_UNIQUE),
                                      svtkm::cont::make_ArrayHandleZip(keyPairIn, keyPairOut));

  SVTKM_TEST_ASSERT(writeKey.GetNumberOfValues() == NUM_UNIQUE,
                   "Reduced values output not sized correctly.");
  CheckPortal(writeKey.GetPortalConstControl());

  CheckPortal(keyPairOut.GetPortalConstControl());
}

void TestReduceByKey(svtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

  std::cout << "Testing svtkm::Id keys." << std::endl;
  TryKeyType(svtkm::Id());

  std::cout << "Testing svtkm::IdComponent keys." << std::endl;
  TryKeyType(svtkm::IdComponent());

  std::cout << "Testing svtkm::UInt8 keys." << std::endl;
  TryKeyType(svtkm::UInt8());

  std::cout << "Testing svtkm::Id3 keys." << std::endl;
  TryKeyType(svtkm::Id3());
}

} // anonymous namespace

int UnitTestWorkletReduceByKey(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(TestReduceByKey, argc, argv);
}
