//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/testing/VecTraitsTests.h>

#include <svtkm/testing/Testing.h>

namespace
{

static constexpr svtkm::Id MAX_VECTOR_SIZE = 5;
static constexpr svtkm::Id VecInit[MAX_VECTOR_SIZE] = { 42, 54, 67, 12, 78 };

void ExpectTrueType(std::true_type)
{
}

void ExpectFalseType(std::false_type)
{
}

struct TypeWithoutVecTraits
{
};

struct TestVecTypeFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    // Make sure that VecTraits actually exists
    ExpectTrueType(svtkm::HasVecTraits<T>());

    using Traits = svtkm::VecTraits<T>;
    using ComponentType = typename Traits::ComponentType;
    SVTKM_TEST_ASSERT(Traits::NUM_COMPONENTS <= MAX_VECTOR_SIZE,
                     "Need to update test for larger vectors.");
    T inVector;
    for (svtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; index++)
    {
      Traits::SetComponent(inVector, index, ComponentType(VecInit[index]));
    }
    T outVector;
    svtkm::testing::TestVecType<Traits::NUM_COMPONENTS>(inVector, outVector);
    svtkm::VecC<ComponentType> outVecC(outVector);
    svtkm::testing::TestVecType<Traits::NUM_COMPONENTS>(svtkm::VecC<ComponentType>(inVector),
                                                       outVecC);
    svtkm::VecCConst<ComponentType> outVecCConst(outVector);
    svtkm::testing::TestVecType<Traits::NUM_COMPONENTS>(svtkm::VecCConst<ComponentType>(inVector),
                                                       outVecCConst);
  }
};

void TestVecTraits()
{
  TestVecTypeFunctor test;
  svtkm::testing::Testing::TryTypes(test);
  std::cout << "svtkm::Vec<svtkm::FloatDefault, 5>" << std::endl;
  test(svtkm::Vec<svtkm::FloatDefault, 5>());

  ExpectFalseType(svtkm::HasVecTraits<TypeWithoutVecTraits>());

  svtkm::testing::TestVecComponentsTag<svtkm::Id3>();
  svtkm::testing::TestVecComponentsTag<svtkm::Vec3f>();
  svtkm::testing::TestVecComponentsTag<svtkm::Vec4f>();
  svtkm::testing::TestVecComponentsTag<svtkm::VecC<svtkm::FloatDefault>>();
  svtkm::testing::TestVecComponentsTag<svtkm::VecCConst<svtkm::Id>>();
  svtkm::testing::TestScalarComponentsTag<svtkm::Id>();
  svtkm::testing::TestScalarComponentsTag<svtkm::FloatDefault>();
}

} // anonymous namespace

int UnitTestVecTraits(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestVecTraits, argc, argv);
}
