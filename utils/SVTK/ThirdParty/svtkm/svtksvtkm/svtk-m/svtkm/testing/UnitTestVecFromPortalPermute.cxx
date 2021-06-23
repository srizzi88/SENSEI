//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/VecFromPortalPermute.h>

#include <svtkm/VecVariable.h>

#include <svtkm/testing/Testing.h>

namespace UnitTestVecFromPortalPermuteNamespace
{

static constexpr svtkm::IdComponent ARRAY_SIZE = 10;

template <typename T>
void CheckType(T, T)
{
  // Check passes if this function is called correctly.
}

template <typename T>
class TestPortal
{
public:
  using ValueType = T;

  SVTKM_EXEC
  svtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  SVTKM_EXEC
  ValueType Get(svtkm::Id index) const { return TestValue(index, ValueType()); }
};

struct VecFromPortalPermuteTestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    using PortalType = TestPortal<T>;
    using IndexVecType = svtkm::VecVariable<svtkm::Id, ARRAY_SIZE>;
    using VecType = svtkm::VecFromPortalPermute<IndexVecType, PortalType>;
    using TTraits = svtkm::TypeTraits<VecType>;
    using VTraits = svtkm::VecTraits<VecType>;

    std::cout << "Checking VecFromPortal traits" << std::endl;

    // The statements will fail to compile if the traits is not working as
    // expected.
    CheckType(typename TTraits::DimensionalityTag(), svtkm::TypeTraitsVectorTag());
    CheckType(typename VTraits::ComponentType(), T());
    CheckType(typename VTraits::HasMultipleComponents(), svtkm::VecTraitsTagMultipleComponents());
    CheckType(typename VTraits::IsSizeStatic(), svtkm::VecTraitsTagSizeVariable());

    std::cout << "Checking VecFromPortal contents" << std::endl;

    PortalType portal;

    for (svtkm::Id offset = 0; offset < ARRAY_SIZE; offset++)
    {
      for (svtkm::IdComponent length = 0; 2 * length + offset < ARRAY_SIZE; length++)
      {
        IndexVecType indices;
        for (svtkm::IdComponent index = 0; index < length; index++)
        {
          indices.Append(offset + 2 * index);
        }

        VecType vec(&indices, portal);

        SVTKM_TEST_ASSERT(vec.GetNumberOfComponents() == length, "Wrong length.");
        SVTKM_TEST_ASSERT(VTraits::GetNumberOfComponents(vec) == length, "Wrong length.");

        svtkm::Vec<T, ARRAY_SIZE> copyDirect;
        vec.CopyInto(copyDirect);

        svtkm::Vec<T, ARRAY_SIZE> copyTraits;
        VTraits::CopyInto(vec, copyTraits);

        for (svtkm::IdComponent index = 0; index < length; index++)
        {
          T expected = TestValue(2 * index + offset, T());
          SVTKM_TEST_ASSERT(test_equal(vec[index], expected), "Wrong value.");
          SVTKM_TEST_ASSERT(test_equal(VTraits::GetComponent(vec, index), expected), "Wrong value.");
          SVTKM_TEST_ASSERT(test_equal(copyDirect[index], expected), "Wrong copied value.");
          SVTKM_TEST_ASSERT(test_equal(copyTraits[index], expected), "Wrong copied value.");
        }
      }
    }
  }
};

void VecFromPortalPermuteTest()
{
  svtkm::testing::Testing::TryTypes(VecFromPortalPermuteTestFunctor(), svtkm::TypeListCommon());
}

} // namespace UnitTestVecFromPortalPermuteNamespace

int UnitTestVecFromPortalPermute(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(
    UnitTestVecFromPortalPermuteNamespace::VecFromPortalPermuteTest, argc, argv);
}
