//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/TypeTraits.h>

#include <svtkm/VecTraits.h>

#include <svtkm/testing/Testing.h>

namespace
{

struct TypeTraitTest
{
  template <typename T>
  void operator()(T t) const
  {
    // If you get compiler errors here, it could be a TypeTraits instance
    // has missing or malformed tags.
    this->TestDimensionality(t, typename svtkm::TypeTraits<T>::DimensionalityTag());
    this->TestNumeric(t, typename svtkm::TypeTraits<T>::NumericTag());
  }

private:
  template <typename T>
  void TestDimensionality(T, svtkm::TypeTraitsScalarTag) const
  {
    std::cout << "  scalar" << std::endl;
    SVTKM_TEST_ASSERT(svtkm::VecTraits<T>::NUM_COMPONENTS == 1,
                     "Scalar type does not have one component.");
  }
  template <typename T>
  void TestDimensionality(T, svtkm::TypeTraitsVectorTag) const
  {
    std::cout << "  vector" << std::endl;
    SVTKM_TEST_ASSERT(svtkm::VecTraits<T>::NUM_COMPONENTS > 1,
                     "Vector type does not have multiple components.");
  }

  template <typename T>
  void TestNumeric(T, svtkm::TypeTraitsIntegerTag) const
  {
    std::cout << "  integer" << std::endl;
    using VT = typename svtkm::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    SVTKM_TEST_ASSERT(value == 2, "Integer does not round to integer.");
  }
  template <typename T>
  void TestNumeric(T, svtkm::TypeTraitsRealTag) const
  {
    std::cout << "  real" << std::endl;
    using VT = typename svtkm::VecTraits<T>::ComponentType;
    VT value = VT(2.001);
    SVTKM_TEST_ASSERT(test_equal(float(value), float(2.001)),
                     "Real does not hold floaing point number.");
  }
};

static void TestTypeTraits()
{
  TypeTraitTest test;
  svtkm::testing::Testing::TryTypes(test);
  std::cout << "svtkm::Vec<svtkm::FloatDefault, 5>" << std::endl;
  test(svtkm::Vec<svtkm::FloatDefault, 5>());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestTypeTraits(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestTypeTraits, argc, argv);
}
