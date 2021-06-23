//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Hash.h>

#include <svtkm/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

SVTKM_CONT
static void CheckUnique(std::vector<svtkm::HashType> hashes)
{
  std::sort(hashes.begin(), hashes.end());

  for (std::size_t index = 1; index < hashes.size(); ++index)
  {
    SVTKM_TEST_ASSERT(hashes[index - 1] != hashes[index], "Found duplicate hashes.");
  }
}

template <typename VecType>
SVTKM_CONT static void DoHashTest(VecType&&)
{
  std::cout << "Test hash for " << svtkm::testing::TypeName<VecType>::Name() << std::endl;

  const svtkm::Id NUM_HASHES = 100;
  std::cout << "  Make sure the first " << NUM_HASHES << " values are unique." << std::endl;
  // There is a small probability that two values of these 100 could be the same. If this test
  // fails we could just be unlucky (and have to use a different set of 100 hashes), but it is
  // suspicious and you should double check the hashes.
  std::vector<svtkm::HashType> hashes;
  hashes.reserve(NUM_HASHES);
  for (svtkm::Id index = 0; index < NUM_HASHES; ++index)
  {
    hashes.push_back(svtkm::Hash(TestValue(index, VecType())));
  }
  CheckUnique(hashes);

  std::cout << "  Try close values that should have different hashes." << std::endl;
  hashes.clear();
  VecType vec = TestValue(5, VecType());
  hashes.push_back(svtkm::Hash(vec));
  vec[0] = vec[0] + 1;
  hashes.push_back(svtkm::Hash(vec));
  vec[1] = vec[1] - 1;
  hashes.push_back(svtkm::Hash(vec));
  CheckUnique(hashes);
}

SVTKM_CONT
static void TestHash()
{
  DoHashTest(svtkm::Id2());
  DoHashTest(svtkm::Id3());
  DoHashTest(svtkm::Vec<svtkm::Id, 10>());
  DoHashTest(svtkm::IdComponent2());
  DoHashTest(svtkm::IdComponent3());
  DoHashTest(svtkm::Vec<svtkm::IdComponent, 10>());
}

} // anonymous namespace

int UnitTestHash(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestHash, argc, argv);
}
