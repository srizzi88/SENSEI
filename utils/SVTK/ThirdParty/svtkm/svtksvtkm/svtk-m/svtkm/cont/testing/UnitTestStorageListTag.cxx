//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This tests deprecated code until it is deleted.

#include <svtkm/cont/StorageListTag.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

SVTKM_DEPRECATED_SUPPRESS_BEGIN

namespace
{

enum TypeId
{
  BASIC
};

TypeId GetTypeId(svtkm::cont::StorageTagBasic)
{
  return BASIC;
}

struct TestFunctor
{
  std::vector<TypeId> FoundTypes;

  template <typename T>
  SVTKM_CONT void operator()(T)
  {
    this->FoundTypes.push_back(GetTypeId(T()));
  }
};

template <svtkm::IdComponent N>
void CheckSame(const svtkm::Vec<TypeId, N>& expected, const std::vector<TypeId>& found)
{
  SVTKM_TEST_ASSERT(static_cast<svtkm::IdComponent>(found.size()) == N, "Got wrong number of items.");

  for (svtkm::IdComponent index = 0; index < N; index++)
  {
    svtkm::UInt32 i = static_cast<svtkm::UInt32>(index);
    SVTKM_TEST_ASSERT(expected[index] == found[i], "Got wrong type.");
  }
}

template <svtkm::IdComponent N, typename ListTag>
void TryList(const svtkm::Vec<TypeId, N>& expected, ListTag)
{
  TestFunctor functor;
  svtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);
}

void TestLists()
{
  std::cout << "StorageListTagBasic" << std::endl;
  TryList(svtkm::Vec<TypeId, 1>(BASIC), svtkm::cont::StorageListTagBasic());
}

} // anonymous namespace

int UnitTestStorageListTag(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestLists, argc, argv);
}

SVTKM_DEPRECATED_SUPPRESS_END
