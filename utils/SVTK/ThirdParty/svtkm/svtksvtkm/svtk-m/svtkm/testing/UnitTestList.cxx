//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/List.h>

#include <svtkm/testing/Testing.h>

#include <vector>

namespace
{

template <int N>
struct TestClass
{
};

} // anonymous namespace

namespace svtkm
{
namespace testing
{

template <int N>
struct TypeName<TestClass<N>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "TestClass<" << N << ">";
    return stream.str();
  }
};
}
} // namespace svtkm::testing

namespace
{

template <typename T>
struct DoubleTransformLazy;
template <int N>
struct DoubleTransformLazy<TestClass<N>>
{
  using type = TestClass<2 * N>;
};

template <typename T>
using DoubleTransform = typename DoubleTransformLazy<T>::type;

template <typename T>
struct EvenPredicate;
template <int N>
struct EvenPredicate<TestClass<N>> : std::integral_constant<bool, (N % 2) == 0>
{
};

template <typename T1, typename T2>
void CheckSame(T1, T2)
{
  SVTKM_STATIC_ASSERT((std::is_same<T1, T2>::value));

  std::cout << "     Got expected type: " << svtkm::testing::TypeName<T1>::Name() << std::endl;
}

template <typename ExpectedList, typename List>
void CheckList(ExpectedList, List)
{
  SVTKM_IS_LIST(List);
  CheckSame(ExpectedList{}, List{});
}

template <int N>
int test_number(TestClass<N>)
{
  return N;
}

template <typename T>
struct MutableFunctor
{
  std::vector<T> FoundTypes;

  template <typename U>
  SVTKM_CONT void operator()(U u)
  {
    this->FoundTypes.push_back(test_number(u));
  }
};

template <typename T>
struct ConstantFunctor
{
  template <typename U, typename VectorType>
  SVTKM_CONT void operator()(U u, VectorType& vector) const
  {
    vector.push_back(test_number(u));
  }
};

void TryForEach()
{
  using TestList =
    svtkm::List<TestClass<1>, TestClass<1>, TestClass<2>, TestClass<3>, TestClass<5>, TestClass<8>>;
  const std::vector<int> expectedList = { 1, 1, 2, 3, 5, 8 };

  std::cout << "Check mutable for each" << std::endl;
  MutableFunctor<int> functor;
  svtkm::ListForEach(functor, TestList{});
  SVTKM_TEST_ASSERT(expectedList == functor.FoundTypes);

  std::cout << "Check constant for each" << std::endl;
  std::vector<int> foundTypes;
  svtkm::ListForEach(ConstantFunctor<int>{}, TestList{}, foundTypes);
  SVTKM_TEST_ASSERT(expectedList == foundTypes);
}

void TestLists()
{
  using SimpleCount = svtkm::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>>;
  using EvenList = svtkm::List<TestClass<2>, TestClass<4>, TestClass<6>, TestClass<8>>;
  using LongList = svtkm::List<TestClass<1>,
                              TestClass<2>,
                              TestClass<3>,
                              TestClass<4>,
                              TestClass<5>,
                              TestClass<6>,
                              TestClass<7>,
                              TestClass<8>,
                              TestClass<9>,
                              TestClass<10>,
                              TestClass<11>,
                              TestClass<12>,
                              TestClass<13>,
                              TestClass<14>>;
  using RepeatList = svtkm::List<TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<1>,
                                TestClass<14>>;

  TryForEach();

  std::cout << "Valid List Tag Checks" << std::endl;
  SVTKM_TEST_ASSERT(svtkm::internal::IsList<svtkm::List<TestClass<11>>>::value);
  SVTKM_TEST_ASSERT(svtkm::internal::IsList<svtkm::List<TestClass<21>, TestClass<22>>>::value);
  SVTKM_TEST_ASSERT(svtkm::internal::IsList<svtkm::ListEmpty>::value);
  SVTKM_TEST_ASSERT(svtkm::internal::IsList<svtkm::ListUniversal>::value);

  std::cout << "ListEmpty" << std::endl;
  CheckList(svtkm::List<>{}, svtkm::ListEmpty{});

  std::cout << "ListAppend" << std::endl;
  CheckList(svtkm::List<TestClass<31>,
                       TestClass<32>,
                       TestClass<33>,
                       TestClass<11>,
                       TestClass<21>,
                       TestClass<22>>{},
            svtkm::ListAppend<svtkm::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                             svtkm::List<TestClass<11>>,
                             svtkm::List<TestClass<21>, TestClass<22>>>{});

  std::cout << "ListIntersect" << std::endl;
  CheckList(svtkm::List<TestClass<3>, TestClass<5>>{},
            svtkm::ListIntersect<
              svtkm::List<TestClass<1>, TestClass<2>, TestClass<3>, TestClass<4>, TestClass<5>>,
              svtkm::List<TestClass<3>, TestClass<5>, TestClass<6>>>{});
  CheckList(svtkm::List<TestClass<1>, TestClass<2>>{},
            svtkm::ListIntersect<svtkm::List<TestClass<1>, TestClass<2>>, svtkm::ListUniversal>{});
  CheckList(svtkm::List<TestClass<1>, TestClass<2>>{},
            svtkm::ListIntersect<svtkm::ListUniversal, svtkm::List<TestClass<1>, TestClass<2>>>{});

  std::cout << "ListTransform" << std::endl;
  CheckList(EvenList{}, svtkm::ListTransform<SimpleCount, DoubleTransform>{});

  std::cout << "ListRemoveIf" << std::endl;
  CheckList(svtkm::List<TestClass<1>, TestClass<3>>{},
            svtkm::ListRemoveIf<SimpleCount, EvenPredicate>{});

  std::cout << "ListSize" << std::endl;
  SVTKM_TEST_ASSERT(svtkm::ListSize<svtkm::ListEmpty>::value == 0);
  SVTKM_TEST_ASSERT(svtkm::ListSize<svtkm::List<TestClass<2>>>::value == 1);
  SVTKM_TEST_ASSERT(svtkm::ListSize<svtkm::List<TestClass<2>, TestClass<4>>>::value == 2);

  std::cout << "ListCross" << std::endl;
  CheckList(svtkm::List<svtkm::List<TestClass<31>, TestClass<11>>,
                       svtkm::List<TestClass<32>, TestClass<11>>,
                       svtkm::List<TestClass<33>, TestClass<11>>>{},
            svtkm::ListCross<svtkm::List<TestClass<31>, TestClass<32>, TestClass<33>>,
                            svtkm::List<TestClass<11>>>{});

  std::cout << "ListAt" << std::endl;
  CheckSame(TestClass<2>{}, svtkm::ListAt<EvenList, 0>{});
  CheckSame(TestClass<4>{}, svtkm::ListAt<EvenList, 1>{});
  CheckSame(TestClass<6>{}, svtkm::ListAt<EvenList, 2>{});
  CheckSame(TestClass<8>{}, svtkm::ListAt<EvenList, 3>{});

  std::cout << "ListIndexOf" << std::endl;
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<EvenList, TestClass<2>>::value == 0);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<EvenList, TestClass<4>>::value == 1);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<EvenList, TestClass<6>>::value == 2);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<EvenList, TestClass<8>>::value == 3);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<EvenList, TestClass<1>>::value == -1);

  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<1>>::value == 0);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<2>>::value == 1);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<3>>::value == 2);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<4>>::value == 3);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<5>>::value == 4);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<6>>::value == 5);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<7>>::value == 6);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<8>>::value == 7);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<9>>::value == 8);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<10>>::value == 9);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<11>>::value == 10);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<12>>::value == 11);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<13>>::value == 12);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<14>>::value == 13);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<15>>::value == -1);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<LongList, TestClass<0>>::value == -1);

  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<RepeatList, TestClass<0>>::value == -1);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<RepeatList, TestClass<1>>::value == 0);
  SVTKM_TEST_ASSERT(svtkm::ListIndexOf<RepeatList, TestClass<14>>::value == 13);

  std::cout << "ListHas" << std::endl;
  SVTKM_TEST_ASSERT(svtkm::ListHas<EvenList, TestClass<2>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<EvenList, TestClass<4>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<EvenList, TestClass<6>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<EvenList, TestClass<8>>::value);
  SVTKM_TEST_ASSERT(!svtkm::ListHas<EvenList, TestClass<1>>::value);

  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<1>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<2>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<3>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<4>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<5>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<6>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<7>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<7>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<8>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<9>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<10>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<11>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<12>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<13>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<LongList, TestClass<14>>::value);
  SVTKM_TEST_ASSERT(!svtkm::ListHas<LongList, TestClass<15>>::value);
  SVTKM_TEST_ASSERT(!svtkm::ListHas<LongList, TestClass<0>>::value);

  SVTKM_TEST_ASSERT(!svtkm::ListHas<RepeatList, TestClass<0>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<RepeatList, TestClass<1>>::value);
  SVTKM_TEST_ASSERT(svtkm::ListHas<RepeatList, TestClass<14>>::value);
}

} // anonymous namespace

int UnitTestList(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestLists, argc, argv);
}
