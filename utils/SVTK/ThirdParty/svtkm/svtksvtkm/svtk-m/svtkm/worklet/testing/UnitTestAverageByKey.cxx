//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/AverageByKey.h>

#include <svtkm/Hash.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCounting.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id NUM_UNIQUE = 100;
static constexpr svtkm::Id NUM_PER_GROUP = 10;
static constexpr svtkm::Id ARRAY_SIZE = NUM_UNIQUE * NUM_PER_GROUP;

template <typename KeyArray, typename ValueArray>
void CheckAverageByKey(const KeyArray& uniqueKeys, const ValueArray& averagedValues)
{
  SVTKM_IS_ARRAY_HANDLE(KeyArray);
  SVTKM_IS_ARRAY_HANDLE(ValueArray);

  using KeyType = typename KeyArray::ValueType;

  SVTKM_TEST_ASSERT(uniqueKeys.GetNumberOfValues() == NUM_UNIQUE, "Bad number of keys.");
  SVTKM_TEST_ASSERT(averagedValues.GetNumberOfValues() == NUM_UNIQUE, "Bad number of values.");

  // We expect the unique keys to be sorted, and for the test values to be in order.
  auto keyPortal = uniqueKeys.GetPortalConstControl();
  auto valuePortal = averagedValues.GetPortalConstControl();
  for (svtkm::Id index = 0; index < NUM_UNIQUE; ++index)
  {
    SVTKM_TEST_ASSERT(keyPortal.Get(index) == TestValue(index % NUM_UNIQUE, KeyType()),
                     "Unexpected key.");

    svtkm::FloatDefault expectedAverage = static_cast<svtkm::FloatDefault>(
      NUM_PER_GROUP * ((NUM_PER_GROUP - 1) * NUM_PER_GROUP) / 2 + index);
    SVTKM_TEST_ASSERT(test_equal(expectedAverage, valuePortal.Get(index)), "Bad average.");
  }
}

template <typename KeyType>
void TryKeyType(KeyType)
{
  std::cout << "Testing with " << svtkm::testing::TypeName<KeyType>::Name() << " keys." << std::endl;

  // Create key array
  KeyType keyBuffer[ARRAY_SIZE];
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index % NUM_UNIQUE, KeyType());
  }
  svtkm::cont::ArrayHandle<KeyType> keysArray = svtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  // Create Keys object
  svtkm::cont::ArrayHandle<KeyType> sortedKeys;
  svtkm::cont::ArrayCopy(keysArray, sortedKeys);
  svtkm::worklet::Keys<KeyType> keys(sortedKeys);
  SVTKM_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  // Create values array
  svtkm::cont::ArrayHandleCounting<svtkm::FloatDefault> valuesArray(0.0f, 1.0f, ARRAY_SIZE);

  std::cout << "  Try average with Keys object" << std::endl;
  CheckAverageByKey(keys.GetUniqueKeys(), svtkm::worklet::AverageByKey::Run(keys, valuesArray));

  std::cout << "  Try average with device adapter's reduce by keys" << std::endl;
  svtkm::cont::ArrayHandle<KeyType> outputKeys;
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> outputValues;
  svtkm::worklet::AverageByKey::Run(keysArray, valuesArray, outputKeys, outputValues);
  CheckAverageByKey(outputKeys, outputValues);
}

void DoTest()
{
  TryKeyType(svtkm::Id());
  TryKeyType(svtkm::IdComponent());
  TryKeyType(svtkm::UInt8());
  TryKeyType(svtkm::HashType());
  TryKeyType(svtkm::Id3());
}

} // anonymous namespace

int UnitTestAverageByKey(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
