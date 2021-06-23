//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandleIndex.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename T>
SVTKM_CONT void TestValues(const svtkm::cont::ArrayHandle<T>& ah,
                          const std::initializer_list<T>& expected)
{
  auto portal = ah.GetPortalConstControl();
  SVTKM_TEST_ASSERT(expected.size() == static_cast<size_t>(ah.GetNumberOfValues()));
  for (svtkm::Id i = 0; i < ah.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == portal.Get(i));
  }
}

template <typename T>
SVTKM_CONT void TestValues(const std::vector<T>& vec, const std::initializer_list<T>& expected)
{
  SVTKM_TEST_ASSERT(expected.size() == vec.size());
  for (std::size_t i = 0; i < vec.size(); ++i)
  {
    SVTKM_TEST_ASSERT(expected.begin()[static_cast<size_t>(i)] == vec[i]);
  }
}

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << svtkm::testing::TypeName<ValueType>::Name() << std::endl;

  svtkm::cont::ArrayHandle<ValueType> data;
  { // Create the ValueType array.
    svtkm::cont::ArrayHandleIndex values(ARRAY_SIZE);
    svtkm::cont::ArrayCopy(values, data);
  }

  { // ArrayHandle ids
    const std::vector<svtkm::Id> idsVec{ 3, 8, 7 };
    const auto ids = svtkm::cont::make_ArrayHandle(idsVec);
    { // Return vector:
      const std::vector<ValueType> output = svtkm::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      svtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
    { // Pass handle:
      svtkm::cont::ArrayHandle<ValueType> output;
      svtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 3, 8, 7 });
    }
  }

  { // vector ids
    const std::vector<svtkm::Id> ids{ 1, 5, 3, 9 };
    { // Return vector:
      const std::vector<ValueType> output = svtkm::cont::ArrayGetValues(ids, data);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass vector:
      std::vector<ValueType> output;
      svtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
    { // Pass handle:
      svtkm::cont::ArrayHandle<ValueType> output;
      svtkm::cont::ArrayGetValues(ids, data, output);
      TestValues<ValueType>(output, { 1, 5, 3, 9 });
    }
  }

  {   // Initializer list ids
    { // Return vector:
      const std::vector<ValueType> output = svtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass vector:
  std::vector<ValueType> output;
  svtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
{ // Pass handle:
  svtkm::cont::ArrayHandle<ValueType> output;
  svtkm::cont::ArrayGetValues({ 4, 2, 0, 6, 9 }, data, output);
  TestValues<ValueType>(output, { 4, 2, 0, 6, 9 });
}
}

{ // c-array ids
  const std::vector<svtkm::Id> idVec{ 8, 6, 7, 5, 3, 0, 9 };
  const svtkm::Id* ids = idVec.data();
  const svtkm::Id n = static_cast<svtkm::Id>(idVec.size());
  { // Return vector:
    const std::vector<ValueType> output = svtkm::cont::ArrayGetValues(ids, n, data);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass vector:
    std::vector<ValueType> output;
    svtkm::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
  { // Pass handle:
    svtkm::cont::ArrayHandle<ValueType> output;
    svtkm::cont::ArrayGetValues(ids, n, data, output);
    TestValues<ValueType>(output, { 8, 6, 7, 5, 3, 0, 9 });
  }
}


{ // single values
  {
    const ValueType output = svtkm::cont::ArrayGetValue(8, data);
    SVTKM_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
  {
    ValueType output;
    svtkm::cont::ArrayGetValue(8, data, output);
    SVTKM_TEST_ASSERT(output == static_cast<ValueType>(8));
  }
}
}

void Test()
{
  TryCopy<svtkm::Id>();
  TryCopy<svtkm::IdComponent>();
  TryCopy<svtkm::Float32>();
}

} // anonymous namespace

int UnitTestArrayGetValues(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(Test, argc, argv);
}
