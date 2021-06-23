//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Matrix.h>
#include <svtkm/Pair.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/testing/Testing.h>

namespace
{

template <typename T, typename U>
void PairTestConstructors()
{
  std::cout << "test that all the constructors work properly" << std::endl;
  svtkm::Pair<T, U> no_params_pair;
  no_params_pair.first = TestValue(12, T());
  no_params_pair.second = TestValue(34, U());
  svtkm::Pair<T, U> copy_constructor_pair(no_params_pair);
  svtkm::Pair<T, U> assignment_pair = no_params_pair;

  SVTKM_TEST_ASSERT((no_params_pair == copy_constructor_pair),
                   "copy constructor doesn't match default constructor");
  SVTKM_TEST_ASSERT(!(no_params_pair != copy_constructor_pair), "operator != is working properly");

  SVTKM_TEST_ASSERT((no_params_pair == assignment_pair),
                   "assignment constructor doesn't match default constructor");
  SVTKM_TEST_ASSERT(!(no_params_pair != assignment_pair), "operator != is working properly");
}

template <typename T, typename U>
void PairTestValues()
{
  std::cout << "Check assignment of values" << std::endl;
  T a = TestValue(56, T());
  U b = TestValue(78, U());

  svtkm::Pair<T, U> pair_ab(a, b);
  svtkm::Pair<T, U> copy_constructor_pair(pair_ab);
  svtkm::Pair<T, U> assignment_pair = pair_ab;
  svtkm::Pair<T, U> make_p = svtkm::make_Pair(a, b);

  SVTKM_TEST_ASSERT(!(pair_ab != pair_ab), "operator != isn't working properly for svtkm::Pair");
  SVTKM_TEST_ASSERT((pair_ab == pair_ab), "operator == isn't working properly for svtkm::Pair");

  SVTKM_TEST_ASSERT((pair_ab == copy_constructor_pair),
                   "copy constructor doesn't match pair constructor");
  SVTKM_TEST_ASSERT((pair_ab == assignment_pair),
                   "assignment constructor doesn't match pair constructor");

  SVTKM_TEST_ASSERT(copy_constructor_pair.first == a, "first field not set right");
  SVTKM_TEST_ASSERT(assignment_pair.second == b, "second field not set right");

  SVTKM_TEST_ASSERT((pair_ab == make_p), "make_pair function doesn't match pair constructor");
}

template <typename T>
T NextValue(T x)
{
  return x + T(1);
}

template <typename T, typename U>
svtkm::Pair<T, U> NextValue(svtkm::Pair<T, U> x)
{
  return svtkm::make_Pair(NextValue(x.first), NextValue(x.second));
}

template <typename T, typename U>
void PairTestOrdering()
{
  std::cout << "Check that ordering operations work" << std::endl;
  //in all cases pair_ab2 is > pair_ab. these verify that if the second
  //argument of the pair is different we respond properly
  T a = TestValue(67, T());
  U b = TestValue(89, U());

  U b2(b);
  svtkm::VecTraits<U>::SetComponent(b2, 0, NextValue(svtkm::VecTraits<U>::GetComponent(b2, 0)));

  svtkm::Pair<T, U> pair_ab2(a, b2);
  svtkm::Pair<T, U> pair_ab(a, b);

  SVTKM_TEST_ASSERT((pair_ab2 >= pair_ab), "operator >= failed");
  SVTKM_TEST_ASSERT((pair_ab2 >= pair_ab2), "operator >= failed");

  SVTKM_TEST_ASSERT((pair_ab2 > pair_ab), "operator > failed");
  SVTKM_TEST_ASSERT(!(pair_ab2 > pair_ab2), "operator > failed");

  SVTKM_TEST_ASSERT(!(pair_ab2 < pair_ab), "operator < failed");
  SVTKM_TEST_ASSERT(!(pair_ab2 < pair_ab2), "operator < failed");

  SVTKM_TEST_ASSERT(!(pair_ab2 <= pair_ab), "operator <= failed");
  SVTKM_TEST_ASSERT((pair_ab2 <= pair_ab2), "operator <= failed");

  SVTKM_TEST_ASSERT(!(pair_ab2 == pair_ab), "operator == failed");
  SVTKM_TEST_ASSERT((pair_ab2 != pair_ab), "operator != failed");

  T a2(a);
  svtkm::VecTraits<T>::SetComponent(a2, 0, NextValue(svtkm::VecTraits<T>::GetComponent(a2, 0)));
  svtkm::Pair<T, U> pair_a2b(a2, b);
  //this way can verify that if the first argument of the pair is different
  //we respond properly
  SVTKM_TEST_ASSERT((pair_a2b >= pair_ab), "operator >= failed");
  SVTKM_TEST_ASSERT((pair_a2b >= pair_a2b), "operator >= failed");

  SVTKM_TEST_ASSERT((pair_a2b > pair_ab), "operator > failed");
  SVTKM_TEST_ASSERT(!(pair_a2b > pair_a2b), "operator > failed");

  SVTKM_TEST_ASSERT(!(pair_a2b < pair_ab), "operator < failed");
  SVTKM_TEST_ASSERT(!(pair_a2b < pair_a2b), "operator < failed");

  SVTKM_TEST_ASSERT(!(pair_a2b <= pair_ab), "operator <= failed");
  SVTKM_TEST_ASSERT((pair_a2b <= pair_a2b), "operator <= failed");

  SVTKM_TEST_ASSERT(!(pair_a2b == pair_ab), "operator == failed");
  SVTKM_TEST_ASSERT((pair_a2b != pair_ab), "operator != failed");
}

//general pair test
template <typename T, typename U>
void PairTest()
{
  {
    using P = svtkm::Pair<T, U>;

    // Pair types should preserve the trivial properties of their components.
    // This insures that algorithms like std::copy will optimize fully.
    SVTKM_TEST_ASSERT(std::is_trivial<T>::value &&
                       std::is_trivial<U>::value == std::is_trivial<P>::value,
                     "PairType's triviality differs from ComponentTypes.");
  }

  PairTestConstructors<T, U>();

  PairTestValues<T, U>();

  PairTestOrdering<T, U>();
}

using PairTypesToTry = svtkm::List<svtkm::Int8,                             // Integer types
                                  svtkm::FloatDefault,                     // Float types
                                  svtkm::Id3,                              // Vec types
                                  svtkm::Pair<svtkm::Vec3f_32, svtkm::Int64> // Recursive Pairs
                                  >;

template <typename FirstType>
struct DecideSecondType
{
  template <typename SecondType>
  void operator()(const SecondType&) const
  {
    PairTest<FirstType, SecondType>();
  }
};

struct DecideFirstType
{
  template <typename T>
  void operator()(const T&) const
  {
    //T is our first type for svtkm::Pair, now to figure out the second type
    svtkm::testing::Testing::TryTypes(DecideSecondType<T>(), PairTypesToTry());
  }
};

void TestPair()
{
  //we want to test each combination of standard svtkm types in a
  //svtkm::Pair, so to do that we dispatch twice on TryTypes. We could
  //dispatch on all types, but that gets excessively large and takes a
  //long time to compile (although it runs fast). Instead, just select
  //a subset of non-trivial combinations.
  svtkm::testing::Testing::TryTypes(DecideFirstType(), PairTypesToTry());
}

} // anonymous namespace

int UnitTestPair(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestPair, argc, argv);
}
