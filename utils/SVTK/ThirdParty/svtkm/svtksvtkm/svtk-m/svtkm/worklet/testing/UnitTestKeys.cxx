//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/Keys.h>

#include <svtkm/cont/ArrayCopy.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 1033;
static constexpr svtkm::Id NUM_UNIQUE = ARRAY_SIZE / 10;

template <typename KeyPortal, typename IdPortal, typename IdComponentPortal>
void CheckKeyReduce(const KeyPortal& originalKeys,
                    const KeyPortal& uniqueKeys,
                    const IdPortal& sortedValuesMap,
                    const IdPortal& offsets,
                    const IdComponentPortal& counts)
{
  using KeyType = typename KeyPortal::ValueType;
  svtkm::Id originalSize = originalKeys.GetNumberOfValues();
  svtkm::Id uniqueSize = uniqueKeys.GetNumberOfValues();
  SVTKM_TEST_ASSERT(originalSize == sortedValuesMap.GetNumberOfValues(), "Inconsistent array size.");
  SVTKM_TEST_ASSERT(uniqueSize == offsets.GetNumberOfValues(), "Inconsistent array size.");
  SVTKM_TEST_ASSERT(uniqueSize == counts.GetNumberOfValues(), "Inconsistent array size.");

  for (svtkm::Id uniqueIndex = 0; uniqueIndex < uniqueSize; uniqueIndex++)
  {
    KeyType key = uniqueKeys.Get(uniqueIndex);
    svtkm::Id offset = offsets.Get(uniqueIndex);
    svtkm::IdComponent groupCount = counts.Get(uniqueIndex);
    for (svtkm::IdComponent groupIndex = 0; groupIndex < groupCount; groupIndex++)
    {
      svtkm::Id originalIndex = sortedValuesMap.Get(offset + groupIndex);
      KeyType originalKey = originalKeys.Get(originalIndex);
      SVTKM_TEST_ASSERT(key == originalKey, "Bad key lookup.");
    }
  }
}

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
  SVTKM_TEST_ASSERT(keys.GetInputRange() == NUM_UNIQUE, "Keys has bad input range.");

  CheckKeyReduce(keyArray.GetPortalConstControl(),
                 keys.GetUniqueKeys().GetPortalConstControl(),
                 keys.GetSortedValuesMap().GetPortalConstControl(),
                 keys.GetOffsets().GetPortalConstControl(),
                 keys.GetCounts().GetPortalConstControl());
}

void TestKeys()
{
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

int UnitTestKeys(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestKeys, argc, argv);
}
