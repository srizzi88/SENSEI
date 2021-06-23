//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/testing/Testing.h>

#include <svtkm/TypeTraits.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/internal/ArrayPortalValueReference.h>

namespace
{

static constexpr svtkm::Id ARRAY_SIZE = 10;

template <typename ArrayPortalType>
void SetReference(svtkm::Id index, svtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  ref = TestValue(index, ValueType());
}

template <typename ArrayPortalType>
void CheckReference(svtkm::Id index, svtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  SVTKM_TEST_ASSERT(test_equal(ref, TestValue(index, ValueType())), "Got bad value from reference.");
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(svtkm::Id index,
                       svtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                       svtkm::TypeTraitsScalarTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  ValueType expected = TestValue(index, ValueType());
  SVTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  SVTKM_TEST_ASSERT(!(ref < ref));
  SVTKM_TEST_ASSERT(ref < ValueType(expected + ValueType(1)));
  SVTKM_TEST_ASSERT(ValueType(expected - ValueType(1)) < ref);

  SVTKM_TEST_ASSERT(!(ref > ref));
  SVTKM_TEST_ASSERT(ref > ValueType(expected - ValueType(1)));
  SVTKM_TEST_ASSERT(ValueType(expected + ValueType(1)) > ref);

  SVTKM_TEST_ASSERT(ref <= ref);
  SVTKM_TEST_ASSERT(ref <= ValueType(expected + ValueType(1)));
  SVTKM_TEST_ASSERT(ValueType(expected - ValueType(1)) <= ref);

  SVTKM_TEST_ASSERT(ref >= ref);
  SVTKM_TEST_ASSERT(ref >= ValueType(expected - ValueType(1)));
  SVTKM_TEST_ASSERT(ValueType(expected + ValueType(1)) >= ref);
}

template <typename ArrayPortalType>
void TryOperatorsNoVec(svtkm::Id,
                       svtkm::internal::ArrayPortalValueReference<ArrayPortalType>,
                       svtkm::TypeTraitsVectorTag)
{
}

template <typename ArrayPortalType>
void TryOperatorsInt(svtkm::Id index,
                     svtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref,
                     svtkm::TypeTraitsScalarTag,
                     svtkm::TypeTraitsIntegerTag)
{
  using ValueType = typename ArrayPortalType::ValueType;

  const ValueType operand = TestValue(ARRAY_SIZE, ValueType());
  ValueType expected = TestValue(index, ValueType());
  SVTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  SVTKM_TEST_ASSERT((ref % ref) == (expected % expected));
  SVTKM_TEST_ASSERT((ref % expected) == (expected % expected));
  SVTKM_TEST_ASSERT((expected % ref) == (expected % expected));

  SVTKM_TEST_ASSERT((ref ^ ref) == (expected ^ expected));
  SVTKM_TEST_ASSERT((ref ^ expected) == (expected ^ expected));
  SVTKM_TEST_ASSERT((expected ^ ref) == (expected ^ expected));

  SVTKM_TEST_ASSERT((ref | ref) == (expected | expected));
  SVTKM_TEST_ASSERT((ref | expected) == (expected | expected));
  SVTKM_TEST_ASSERT((expected | ref) == (expected | expected));

  SVTKM_TEST_ASSERT((ref & ref) == (expected & expected));
  SVTKM_TEST_ASSERT((ref & expected) == (expected & expected));
  SVTKM_TEST_ASSERT((expected & ref) == (expected & expected));

  SVTKM_TEST_ASSERT((ref << ref) == (expected << expected));
  SVTKM_TEST_ASSERT((ref << expected) == (expected << expected));
  SVTKM_TEST_ASSERT((expected << ref) == (expected << expected));

  SVTKM_TEST_ASSERT((ref << ref) == (expected << expected));
  SVTKM_TEST_ASSERT((ref << expected) == (expected << expected));
  SVTKM_TEST_ASSERT((expected << ref) == (expected << expected));

  SVTKM_TEST_ASSERT(~ref == ~expected);

  SVTKM_TEST_ASSERT(!(!ref));

  SVTKM_TEST_ASSERT(ref && ref);
  SVTKM_TEST_ASSERT(ref && expected);
  SVTKM_TEST_ASSERT(expected && ref);

  SVTKM_TEST_ASSERT(ref || ref);
  SVTKM_TEST_ASSERT(ref || expected);
  SVTKM_TEST_ASSERT(expected || ref);

#if defined(SVTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

  ref &= ref;
  expected &= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref &= operand;
  expected &= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref |= ref;
  expected |= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref |= operand;
  expected |= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref >>= ref;
  expected >>= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref >>= operand;
  expected >>= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref <<= ref;
  expected <<= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref <<= operand;
  expected <<= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref ^= ref;
  expected ^= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref ^= operand;
  expected ^= operand;
  SVTKM_TEST_ASSERT(ref == expected);

#if defined(SVTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic pop
#endif
}

template <typename ArrayPortalType, typename DimTag, typename NumericTag>
void TryOperatorsInt(svtkm::Id,
                     svtkm::internal::ArrayPortalValueReference<ArrayPortalType>,
                     DimTag,
                     NumericTag)
{
}

template <typename ArrayPortalType>
void TryOperators(svtkm::Id index, svtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;

  const ValueType operand = TestValue(ARRAY_SIZE, ValueType());
  ValueType expected = TestValue(index, ValueType());
  SVTKM_TEST_ASSERT(ref.Get() == expected, "Reference did not start out as expected.");

  // Test comparison operators.
  SVTKM_TEST_ASSERT(ref == ref);
  SVTKM_TEST_ASSERT(ref == expected);
  SVTKM_TEST_ASSERT(expected == ref);

  SVTKM_TEST_ASSERT(!(ref != ref));
  SVTKM_TEST_ASSERT(!(ref != expected));
  SVTKM_TEST_ASSERT(!(expected != ref));

  TryOperatorsNoVec(index, ref, typename svtkm::TypeTraits<ValueType>::DimensionalityTag());

  SVTKM_TEST_ASSERT((ref + ref) == (expected + expected));
  SVTKM_TEST_ASSERT((ref + expected) == (expected + expected));
  SVTKM_TEST_ASSERT((expected + ref) == (expected + expected));

  SVTKM_TEST_ASSERT((ref - ref) == (expected - expected));
  SVTKM_TEST_ASSERT((ref - expected) == (expected - expected));
  SVTKM_TEST_ASSERT((expected - ref) == (expected - expected));

  SVTKM_TEST_ASSERT((ref * ref) == (expected * expected));
  SVTKM_TEST_ASSERT((ref * expected) == (expected * expected));
  SVTKM_TEST_ASSERT((expected * ref) == (expected * expected));

  SVTKM_TEST_ASSERT((ref / ref) == (expected / expected));
  SVTKM_TEST_ASSERT((ref / expected) == (expected / expected));
  SVTKM_TEST_ASSERT((expected / ref) == (expected / expected));


#if defined(SVTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif

  ref += ref;
  expected += expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref += operand;
  expected += operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref -= ref;
  expected -= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref -= operand;
  expected -= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref *= ref;
  expected *= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref *= operand;
  expected *= operand;
  SVTKM_TEST_ASSERT(ref == expected);

  ref /= ref;
  expected /= expected;
  SVTKM_TEST_ASSERT(ref == expected);
  ref /= operand;
  expected /= operand;
  SVTKM_TEST_ASSERT(ref == expected);

#if defined(SVTKM_CLANG) && __clang_major__ >= 7
#pragma clang diagnostic pop
#endif

  // Reset ref
  ref = TestValue(index, ValueType());

  TryOperatorsInt(index,
                  ref,
                  typename svtkm::TypeTraits<ValueType>::DimensionalityTag(),
                  typename svtkm::TypeTraits<ValueType>::NumericTag());
}

struct DoTestForType
{
  template <typename ValueType>
  SVTKM_CONT void operator()(const ValueType&) const
  {
    svtkm::cont::ArrayHandle<ValueType> array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Set array using reference" << std::endl;
    using PortalType = typename svtkm::cont::ArrayHandle<ValueType>::PortalControl;
    PortalType portal = array.GetPortalControl();
    for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      SetReference(index, svtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check values" << std::endl;
    CheckPortal(portal);

    std::cout << "Check references in set array." << std::endl;
    for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      CheckReference(index, svtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check that operators work." << std::endl;
    // Start at 1 to avoid issues with 0.
    for (svtkm::Id index = 1; index < ARRAY_SIZE; ++index)
    {
      TryOperators(index, svtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }
  }
};

void DoTest()
{
  // We are not testing on the default (exemplar) types because they include unsigned bytes, and
  // simply doing a += (or similar) operation on them automatically creates a conversion warning
  // on some compilers. Since we want to test these operators, just remove the short types from
  // the list to avoid the warning.
  svtkm::testing::Testing::TryTypes(DoTestForType(),
                                   svtkm::List<svtkm::Id, svtkm::FloatDefault, svtkm::Vec3f_64>());
}

} // anonymous namespace

int UnitTestArrayPortalValueReference(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(DoTest, argc, argv);
}
