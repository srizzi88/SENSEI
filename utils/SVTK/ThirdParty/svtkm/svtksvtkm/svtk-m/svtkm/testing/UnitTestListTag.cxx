//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// ListTag has been depricated. Until it is officially removed, we continue to test it, but
// disable the deprecated warnings while doing so. Once ListTag is officially removed,
// this entire test can be deleted.
#include <svtkm/Deprecated.h>
SVTKM_DEPRECATED_SUPPRESS_BEGIN

#include <svtkm/ListTag.h>

#include <svtkm/Types.h>

#include <svtkm/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template <int N>
struct TestClass
{
};

struct TestListTag1 : svtkm::ListTagBase<TestClass<11>>
{
};
using TestListTagBackward1 = svtkm::List<TestClass<11>>;

struct TestListTag2 : svtkm::ListTagBase<TestClass<21>, TestClass<22>>
{
};
using TestListTagBackward2 = svtkm::List<TestClass<21>, TestClass<22>>;

struct TestListTag3 : svtkm::ListTagBase<TestClass<31>, TestClass<32>, TestClass<33>>
{
};
using TestListTagBackward3 = svtkm::List<TestClass<31>, TestClass<32>, TestClass<33>>;

struct TestListTag4 : svtkm::ListTagBase<TestClass<41>, TestClass<42>, TestClass<43>, TestClass<44>>
{
};
using TestListTagBackward4 = svtkm::List<TestClass<41>, TestClass<42>, TestClass<43>, TestClass<44>>;

struct TestListTagJoin : svtkm::ListTagJoin<TestListTag3, TestListTag1>
{
};
struct TestListTagJoinBackward : svtkm::ListTagJoin<TestListTagBackward3, TestListTagBackward1>
{
};

struct TestListTagIntersect : svtkm::ListTagIntersect<TestListTag3, TestListTagJoin>
{
};
struct TestListTagIntersectBackward
  : svtkm::ListTagIntersect<TestListTagBackward3, TestListTagJoinBackward>
{
};

struct TestListTagCrossProduct : svtkm::ListCrossProduct<TestListTag3, TestListTag1>
{
};
struct TestListTagCrossProductBackward
  : svtkm::ListCrossProduct<TestListTagBackward3, TestListTagBackward1>
{
};

struct TestListTagUniversal : svtkm::ListTagUniversal
{
};

struct TestListTagAppend : svtkm::ListTagAppend<TestListTag3, TestClass<34>>
{
};
struct TestListTagAppendBackward : svtkm::ListTagAppend<TestListTagBackward3, TestClass<34>>
{
};

struct TestListTagAppendUnique1 : svtkm::ListTagAppendUnique<TestListTag3, TestClass<32>>
{
};
struct TestListTagAppendUniqueBackward1
  : svtkm::ListTagAppendUnique<TestListTagBackward3, TestClass<32>>
{
};

struct TestListTagAppendUnique2 : svtkm::ListTagAppendUnique<TestListTagAppendUnique1, TestClass<34>>
{
};
struct TestListTagAppendUniqueBackward2
  : svtkm::ListTagAppendUnique<TestListTagAppendUniqueBackward1, TestClass<34>>
{
};

template <typename T>
struct DoubleTransformImpl;
template <int N>
struct DoubleTransformImpl<TestClass<N>>
{
  using type = TestClass<2 * N>;
};

template <typename T>
using DoubleTransform = typename DoubleTransformImpl<T>::type;

struct TestListTagTransform : svtkm::ListTagTransform<TestListTag4, DoubleTransform>
{
};
struct TestListTagTransformBackward : svtkm::ListTagTransform<TestListTagBackward4, DoubleTransform>
{
};

template <typename T>
struct EvenPredicate;
template <int N>
struct EvenPredicate<TestClass<N>> : std::integral_constant<bool, (N % 2) == 0>
{
};

struct TestListTagRemoveIf : svtkm::ListTagRemoveIf<TestListTag4, EvenPredicate>
{
};
struct TestListTagRemoveIfBackward : svtkm::ListTagRemoveIf<TestListTagBackward4, EvenPredicate>
{
};

template <int N, int M>
std::pair<int, int> test_number(brigand::list<TestClass<N>, TestClass<M>>)
{
  return std::make_pair(N, M);
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

template <typename T, svtkm::IdComponent N>
void CheckSame(const svtkm::Vec<T, N>& expected, const std::vector<T>& found)
{
  SVTKM_TEST_ASSERT(static_cast<int>(found.size()) == N, "Got wrong number of items.");

  for (svtkm::IdComponent index = 0; index < N; index++)
  {
    svtkm::UInt32 i = static_cast<svtkm::UInt32>(index);
    SVTKM_TEST_ASSERT(expected[index] == found[i], "Got wrong type.");
  }
}

template <int N, typename ListTag>
void CheckContains(TestClass<N>, ListTag, const std::vector<int>& contents)
{
  bool listContains = svtkm::ListContains<ListTag, TestClass<N>>::value;
  bool shouldContain = std::find(contents.begin(), contents.end(), N) != contents.end();

  SVTKM_TEST_ASSERT(listContains == shouldContain, "ListContains check failed.");
}

template <int N>
void CheckContains(TestClass<N>, TestListTagUniversal, const std::vector<int>&)
{
  //Use intersect to verify at compile time that ListTag contains TestClass<N>
  using intersectWith = svtkm::ListTagBase<TestClass<N>>;
  using intersectResult = svtkm::internal::ListTagAsBrigandList<
    svtkm::ListTagIntersect<intersectWith, TestListTagUniversal>>;
  constexpr bool intersectContains = (brigand::size<intersectResult>::value != 0);
  constexpr bool listContains = svtkm::ListContains<TestListTagUniversal, TestClass<N>>::value;

  SVTKM_TEST_ASSERT(intersectContains == listContains, "ListTagIntersect check failed.");
}

template <svtkm::IdComponent N, typename ListTag>
void TryList(const svtkm::Vec<int, N>& expected, ListTag)
{
  SVTKM_IS_LIST_TAG(ListTag);

  SVTKM_STATIC_ASSERT(svtkm::ListSize<ListTag>::value == N);

  std::cout << "    Try mutable for each" << std::endl;
  MutableFunctor<int> functor;
  svtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);

  std::cout << "    Try constant for each" << std::endl;
  std::vector<int> foundTypes;
  ConstantFunctor<int> cfunc;
  svtkm::ListForEach(cfunc, ListTag(), foundTypes);
  CheckSame(expected, foundTypes);

  std::cout << "    Try checking contents" << std::endl;
  CheckContains(TestClass<11>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<21>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<22>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<31>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<32>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<33>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<41>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<42>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<43>(), ListTag(), functor.FoundTypes);
  CheckContains(TestClass<44>(), ListTag(), functor.FoundTypes);
}
template <svtkm::IdComponent N, typename ListTag>
void TryList(const svtkm::Vec<std::pair<int, int>, N>& expected, ListTag)
{
  SVTKM_IS_LIST_TAG(ListTag);

  std::cout << "    Try mutable for each" << std::endl;
  MutableFunctor<std::pair<int, int>> functor;
  svtkm::ListForEach(functor, ListTag());
  CheckSame(expected, functor.FoundTypes);

  std::cout << "    Try constant for each" << std::endl;
  std::vector<std::pair<int, int>> foundTypes;
  ConstantFunctor<std::pair<int, int>> cfunc;
  svtkm::ListForEach(cfunc, ListTag(), foundTypes);
  CheckSame(expected, foundTypes);
}

template <svtkm::IdComponent N>
void TryList(const svtkm::Vec<int, N>&, TestListTagUniversal tag)
{
  SVTKM_IS_LIST_TAG(TestListTagUniversal);

  //TestListTagUniversal can't be used with for_each on purpose

  std::vector<int> found;
  std::cout << "    Try checking contents" << std::endl;
  CheckContains(TestClass<11>(), tag, found);
  CheckContains(TestClass<21>(), tag, found);
  CheckContains(TestClass<22>(), tag, found);
  CheckContains(TestClass<31>(), tag, found);
  CheckContains(TestClass<32>(), tag, found);
  CheckContains(TestClass<33>(), tag, found);
  CheckContains(TestClass<41>(), tag, found);
  CheckContains(TestClass<42>(), tag, found);
  CheckContains(TestClass<43>(), tag, found);
  CheckContains(TestClass<44>(), tag, found);
}

void TestLists()
{
  std::cout << "Valid List Tag Checks" << std::endl;
  SVTKM_TEST_ASSERT(svtkm::internal::ListTagCheck<TestListTag1>::value, "Failed list tag check");
  SVTKM_TEST_ASSERT(svtkm::internal::ListTagCheck<TestListTagJoin>::value, "Failed list tag check");
  SVTKM_TEST_ASSERT(!svtkm::internal::ListTagCheck<TestClass<1>>::value, "Failed list tag check");

  std::cout << "ListTagEmpty" << std::endl;
  TryList(svtkm::Vec<int, 0>(), svtkm::ListTagEmpty());

  std::cout << "ListTagBase" << std::endl;
  TryList(svtkm::Vec<int, 1>(11), TestListTag1());

  std::cout << "ListTagBase2" << std::endl;
  TryList(svtkm::Vec<int, 2>(21, 22), TestListTag2());

  std::cout << "ListTagBase3" << std::endl;
  TryList(svtkm::Vec<int, 3>(31, 32, 33), TestListTag3());

  std::cout << "ListTagBase4" << std::endl;
  TryList(svtkm::Vec<int, 4>(41, 42, 43, 44), TestListTag4());

  std::cout << "ListTagJoin" << std::endl;
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 11), TestListTagJoin());
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 11), TestListTagJoinBackward());

  std::cout << "ListTagIntersect" << std::endl;
  TryList(svtkm::Vec<int, 3>(31, 32, 33), TestListTagIntersect());
  TryList(svtkm::Vec<int, 3>(31, 32, 33), TestListTagIntersectBackward());

  std::cout << "ListTagCrossProduct" << std::endl;
  TryList(svtkm::Vec<std::pair<int, int>, 3>({ 31, 11 }, { 32, 11 }, { 33, 11 }),
          TestListTagCrossProduct());
  TryList(svtkm::Vec<std::pair<int, int>, 3>({ 31, 11 }, { 32, 11 }, { 33, 11 }),
          TestListTagCrossProductBackward());

  std::cout << "ListTagAppend" << std::endl;
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppend());
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppendBackward());

  std::cout << "ListTagAppendUnique1" << std::endl;
  TryList(svtkm::Vec<int, 3>(31, 32, 33), TestListTagAppendUnique1());
  TryList(svtkm::Vec<int, 3>(31, 32, 33), TestListTagAppendUniqueBackward1());

  std::cout << "ListTagAppendUnique2" << std::endl;
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppendUnique2());
  TryList(svtkm::Vec<int, 4>(31, 32, 33, 34), TestListTagAppendUniqueBackward2());

  std::cout << "ListTagTransform" << std::endl;
  TryList(svtkm::Vec<int, 4>(82, 84, 86, 88), TestListTagTransform());
  TryList(svtkm::Vec<int, 4>(82, 84, 86, 88), TestListTagTransformBackward());

  std::cout << "ListTagRemoveIf" << std::endl;
  TryList(svtkm::Vec<int, 2>(41, 43), TestListTagRemoveIf());
  TryList(svtkm::Vec<int, 2>(41, 43), TestListTagRemoveIfBackward());



  std::cout << "ListTagUniversal" << std::endl;
  TryList(svtkm::Vec<int, 4>(1, 2, 3, 4), TestListTagUniversal());
}

} // anonymous namespace

int UnitTestListTag(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestLists, argc, argv);
}

SVTKM_DEPRECATED_SUPPRESS_END
