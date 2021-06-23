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
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleIndex.h>

#include <svtkm/TypeTraits.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename RefPortalType, typename TestPortalType>
void TestValues(const RefPortalType& refPortal, const TestPortalType& testPortal)
{
  const svtkm::Id arraySize = refPortal.GetNumberOfValues();
  SVTKM_TEST_ASSERT(arraySize == testPortal.GetNumberOfValues(), "Wrong array size.");

  for (svtkm::Id index = 0; index < arraySize; ++index)
  {
    SVTKM_TEST_ASSERT(test_equal(refPortal.Get(index), testPortal.Get(index)), "Got bad value.");
  }
}

template <typename ValueType>
void TryCopy()
{
  SVTKM_LOG_S(svtkm::cont::LogLevel::Info,
             "Trying type: " << svtkm::testing::TypeName<ValueType>::Name());

  { // implicit -> basic
    svtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    svtkm::cont::ArrayHandle<ValueType> output;
    svtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // basic -> basic
    svtkm::cont::ArrayHandleIndex source(ARRAY_SIZE);
    svtkm::cont::ArrayHandle<svtkm::Id> input;
    svtkm::cont::ArrayCopy(source, input);
    svtkm::cont::ArrayHandle<ValueType> output;
    svtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (index)
    svtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
    svtkm::cont::ArrayHandleIndex output;
    svtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (constant)
    svtkm::cont::ArrayHandleConstant<int> input(41, ARRAY_SIZE);
    svtkm::cont::ArrayHandleConstant<int> output;
    svtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }

  { // implicit -> implicit (base->derived, constant)
    svtkm::cont::ArrayHandle<int, svtkm::cont::StorageTagConstant> input =
      svtkm::cont::make_ArrayHandleConstant<int>(41, ARRAY_SIZE);
    svtkm::cont::ArrayHandleConstant<int> output;
    svtkm::cont::ArrayCopy(input, output);
    TestValues(input.GetPortalConstControl(), output.GetPortalConstControl());
  }
}

void TestArrayCopy()
{
  TryCopy<svtkm::Id>();
  TryCopy<svtkm::IdComponent>();
  TryCopy<svtkm::Float32>();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
