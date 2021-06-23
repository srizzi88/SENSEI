//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCartesianProduct.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleExtractComponent.h>
#include <svtkm/cont/ArrayHandleGroupVec.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleReverse.h>
#include <svtkm/cont/ArrayHandleSOA.h>
#include <svtkm/cont/ArrayHandleSwizzle.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <svtkm/cont/ArrayHandleZip.h>

#include <svtkm/cont/VariantArrayHandle.h>

#include <svtkm/cont/testing/TestingSerialization.h>

#include <svtkm/VecTraits.h>

#include <ctime>
#include <type_traits>
#include <vector>

using namespace svtkm::cont::testing::serialization;

namespace
{

//-----------------------------------------------------------------------------
struct TestEqualArrayHandle
{
public:
  template <typename ArrayHandle1, typename ArrayHandle2>
  SVTKM_CONT void operator()(const ArrayHandle1& array1, const ArrayHandle2& array2) const
  {
    auto result = svtkm::cont::testing::test_equal_ArrayHandles(array1, array2);
    SVTKM_TEST_ASSERT(result, result.GetMergedMessage());
  }
};

//-----------------------------------------------------------------------------
template <typename T>
inline void RunTest(const T& obj)
{
  TestSerialization(obj, TestEqualArrayHandle{});
}

//-----------------------------------------------------------------------------
constexpr svtkm::Id ArraySize = 10;

using TestTypesList = svtkm::List<svtkm::Int8, svtkm::Id, svtkm::FloatDefault, svtkm::Vec3f>;

template <typename T, typename S>
inline svtkm::cont::VariantArrayHandleBase<svtkm::ListAppend<TestTypesList, svtkm::List<T>>>
MakeTestVariantArrayHandle(const svtkm::cont::ArrayHandle<T, S>& array)
{
  return array;
}

struct TestArrayHandleBasic
{
  template <typename T>
  void operator()(T) const
  {
    auto array = RandomArrayHandle<T>::Make(ArraySize);
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleSOA
{
  template <typename T>
  void operator()(T) const
  {
    svtkm::cont::ArrayHandleSOA<T> array;
    svtkm::cont::ArrayCopy(RandomArrayHandle<T>::Make(ArraySize), array);
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleCartesianProduct
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      svtkm::cont::make_ArrayHandleCartesianProduct(RandomArrayHandle<T>::Make(ArraySize),
                                                   RandomArrayHandle<T>::Make(ArraySize),
                                                   RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleCast
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      svtkm::cont::make_ArrayHandleCast<T>(RandomArrayHandle<svtkm::Int8>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }

  template <typename T, svtkm::IdComponent N>
  void operator()(svtkm::Vec<T, N>) const
  {
    auto array = svtkm::cont::make_ArrayHandleCast<svtkm::Vec<T, N>>(
      RandomArrayHandle<svtkm::Vec<svtkm::Int8, N>>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleCompositeVector
{
  template <typename T>
  void operator()(T) const
  {
    auto array = svtkm::cont::make_ArrayHandleCompositeVector(RandomArrayHandle<T>::Make(ArraySize),
                                                             RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleConcatenate
{
  template <typename T>
  void operator()(T) const
  {
    auto array = svtkm::cont::make_ArrayHandleConcatenate(RandomArrayHandle<T>::Make(ArraySize),
                                                         RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleConstant
{
  template <typename T>
  void operator()(T) const
  {
    T cval = RandomValue<T>::Make();
    auto array = svtkm::cont::make_ArrayHandleConstant(cval, ArraySize);
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleCounting
{
  template <typename T>
  void operator()(T) const
  {
    T start = RandomValue<T>::Make();
    T step = RandomValue<T>::Make(0, 5);
    auto array = svtkm::cont::make_ArrayHandleCounting(start, step, ArraySize);
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleExtractComponent
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = svtkm::VecTraits<T>::NUM_COMPONENTS;
    auto array = svtkm::cont::make_ArrayHandleExtractComponent(
      RandomArrayHandle<T>::Make(ArraySize), RandomValue<svtkm::IdComponent>::Make(0, numComps - 1));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleGroupVec
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = RandomValue<svtkm::IdComponent>::Make(2, 4);
    auto flat = RandomArrayHandle<T>::Make(ArraySize * numComps);
    switch (numComps)
    {
      case 3:
      {
        auto array = svtkm::cont::make_ArrayHandleGroupVec<3>(flat);
        RunTest(array);
        RunTest(MakeTestVariantArrayHandle(array));
        break;
      }
      case 4:
      {
        auto array = svtkm::cont::make_ArrayHandleGroupVec<4>(flat);
        RunTest(array);
        RunTest(MakeTestVariantArrayHandle(array));
        break;
      }
      default:
      {
        auto array = svtkm::cont::make_ArrayHandleGroupVec<2>(flat);
        RunTest(array);
        RunTest(MakeTestVariantArrayHandle(array));
        break;
      }
    }
  }
};

struct TestArrayHandleGroupVecVariable
{
  template <typename T>
  void operator()(T) const
  {
    auto rangen = UniformRandomValueGenerator<svtkm::IdComponent>(1, 4);
    svtkm::Id size = 0;

    std::vector<svtkm::Id> comps(ArraySize);
    std::generate(comps.begin(), comps.end(), [&size, &rangen]() {
      auto offset = size;
      size += rangen();
      return offset;
    });

    auto array = svtkm::cont::make_ArrayHandleGroupVecVariable(RandomArrayHandle<T>::Make(size),
                                                              svtkm::cont::make_ArrayHandle(comps));
    RunTest(array);

    // cannot make a VariantArrayHandle containing ArrayHandleGroupVecVariable
    // because of the variable number of components of its values.
    // RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleImplicit
{
  template <typename T>
  struct ImplicitFunctor
  {
    ImplicitFunctor() = default;

    explicit ImplicitFunctor(const T& factor)
      : Factor(factor)
    {
    }

    SVTKM_EXEC_CONT T operator()(svtkm::Id index) const
    {
      return static_cast<T>(this->Factor *
                            static_cast<typename svtkm::VecTraits<T>::ComponentType>(index));
    }

    T Factor;
  };

  template <typename T>
  void operator()(T) const
  {
    ImplicitFunctor<T> functor(RandomValue<T>::Make(2, 9));
    auto array = svtkm::cont::make_ArrayHandleImplicit(functor, ArraySize);
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

void TestArrayHandleIndex()
{
  auto size = RandomValue<svtkm::Id>::Make(2, 10);
  auto array = svtkm::cont::ArrayHandleIndex(size);
  RunTest(array);
  RunTest(MakeTestVariantArrayHandle(array));
}

struct TestArrayHandlePermutation
{
  template <typename T>
  void operator()(T) const
  {
    std::uniform_int_distribution<svtkm::Id> distribution(0, ArraySize - 1);

    std::vector<svtkm::Id> inds(ArraySize);
    std::generate(inds.begin(), inds.end(), [&distribution]() { return distribution(generator); });

    auto array = svtkm::cont::make_ArrayHandlePermutation(
      RandomArrayHandle<svtkm::Id>::Make(ArraySize, 0, ArraySize - 1),
      RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleReverse
{
  template <typename T>
  void operator()(T) const
  {
    auto array = svtkm::cont::make_ArrayHandleReverse(RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};

struct TestArrayHandleSwizzle
{
  template <typename T>
  void operator()(T) const
  {
    static const svtkm::IdComponent2 map2s[6] = { { 0, 1 }, { 0, 2 }, { 1, 0 },
                                                 { 1, 2 }, { 2, 0 }, { 2, 1 } };
    static const svtkm::IdComponent3 map3s[6] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 },
                                                 { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };

    auto numOutComps = RandomValue<svtkm::IdComponent>::Make(2, 3);
    switch (numOutComps)
    {
      case 2:
      {
        auto array = make_ArrayHandleSwizzle(RandomArrayHandle<svtkm::Vec<T, 3>>::Make(ArraySize),
                                             map2s[RandomValue<int>::Make(0, 5)]);
        RunTest(array);
        RunTest(MakeTestVariantArrayHandle(array));
        break;
      }
      case 3:
      default:
      {
        auto array = make_ArrayHandleSwizzle(RandomArrayHandle<svtkm::Vec<T, 3>>::Make(ArraySize),
                                             map3s[RandomValue<int>::Make(0, 5)]);
        RunTest(array);
        RunTest(MakeTestVariantArrayHandle(array));
        break;
      }
    }
  }
};


struct TestArrayHandleTransform
{
  struct TransformFunctor
  {
    template <typename T>
    SVTKM_EXEC_CONT T operator()(const T& in) const
    {
      return static_cast<T>(in * T{ 2 });
    }
  };

  struct InverseTransformFunctor
  {
    template <typename T>
    SVTKM_EXEC_CONT T operator()(const T& in) const
    {
      return static_cast<T>(in / T{ 2 });
    }
  };

  template <typename T>
  void TestType1() const
  {
    auto array = svtkm::cont::make_ArrayHandleTransform(RandomArrayHandle<T>::Make(ArraySize),
                                                       TransformFunctor{});
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }

  template <typename T>
  void TestType2() const
  {
    auto array = svtkm::cont::make_ArrayHandleTransform(
      RandomArrayHandle<T>::Make(ArraySize), TransformFunctor{}, InverseTransformFunctor{});
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }

  template <typename T>
  void operator()(T) const
  {
    this->TestType1<T>();
    this->TestType2<T>();
  }
};

svtkm::cont::ArrayHandleUniformPointCoordinates MakeRandomArrayHandleUniformPointCoordinates()
{
  auto dimensions = RandomValue<svtkm::Id3>::Make(1, 3);
  auto origin = RandomValue<svtkm::Vec3f>::Make();
  auto spacing = RandomValue<svtkm::Vec3f>::Make(0.1f, 10.0f);
  return svtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing);
}

void TestArrayHandleUniformPointCoordinates()
{
  auto array = MakeRandomArrayHandleUniformPointCoordinates();
  RunTest(array);
  RunTest(MakeTestVariantArrayHandle(array));
}

void TestArrayHandleVirtualCoordinates()
{
  int type = RandomValue<int>::Make(0, 2);

  svtkm::cont::ArrayHandleVirtualCoordinates array;
  switch (type)
  {
    case 0:
      array =
        svtkm::cont::ArrayHandleVirtualCoordinates(MakeRandomArrayHandleUniformPointCoordinates());
      break;
    case 1:
      array =
        svtkm::cont::ArrayHandleVirtualCoordinates(svtkm::cont::make_ArrayHandleCartesianProduct(
          RandomArrayHandle<svtkm::FloatDefault>::Make(ArraySize),
          RandomArrayHandle<svtkm::FloatDefault>::Make(ArraySize),
          RandomArrayHandle<svtkm::FloatDefault>::Make(ArraySize)));
      break;
    default:
      array =
        svtkm::cont::ArrayHandleVirtualCoordinates(RandomArrayHandle<svtkm::Vec3f>::Make(ArraySize));
      break;
  }

  RunTest(array);
  RunTest(MakeTestVariantArrayHandle(array));
}

struct TestArrayHandleZip
{
  template <typename T>
  void operator()(T) const
  {
    auto array = svtkm::cont::make_ArrayHandleZip(RandomArrayHandle<T>::Make(ArraySize),
                                                 svtkm::cont::ArrayHandleIndex(ArraySize));
    RunTest(array);
    RunTest(MakeTestVariantArrayHandle(array));
  }
};


//-----------------------------------------------------------------------------
void TestArrayHandleSerialization()
{
  std::cout << "Testing ArrayHandleBasic\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleBasic(), TestTypesList());

  std::cout << "Testing ArrayHandleSOA\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleSOA(), TestTypesList());

  std::cout << "Testing ArrayHandleCartesianProduct\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleCartesianProduct(), TestTypesList());

  std::cout << "Testing TestArrayHandleCast\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleCast(), TestTypesList());

  std::cout << "Testing ArrayHandleCompositeVector\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleCompositeVector(), TestTypesList());

  std::cout << "Testing ArrayHandleConcatenate\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleConcatenate(), TestTypesList());

  std::cout << "Testing ArrayHandleConstant\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleConstant(), TestTypesList());

  std::cout << "Testing ArrayHandleCounting\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleCounting(), TestTypesList());

  std::cout << "Testing ArrayHandleExtractComponent\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleExtractComponent(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVec\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleGroupVec(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVecVariable\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleGroupVecVariable(), TestTypesList());

  std::cout << "Testing ArrayHandleImplicit\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleImplicit(), TestTypesList());

  std::cout << "Testing ArrayHandleIndex\n";
  TestArrayHandleIndex();

  std::cout << "Testing ArrayHandlePermutation\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandlePermutation(), TestTypesList());

  std::cout << "Testing ArrayHandleReverse\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleReverse(), TestTypesList());

  std::cout << "Testing ArrayHandleSwizzle\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleSwizzle(), TestTypesList());

  std::cout << "Testing ArrayHandleTransform\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleTransform(), TestTypesList());

  std::cout << "Testing ArrayHandleUniformPointCoordinates\n";
  TestArrayHandleUniformPointCoordinates();

  std::cout << "Testing ArrayHandleVirtualCoordinates\n";
  TestArrayHandleVirtualCoordinates();

  std::cout << "Testing ArrayHandleZip\n";
  svtkm::testing::Testing::TryTypes(TestArrayHandleZip(), TestTypesList());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestSerializationArrayHandle(int argc, char* argv[])
{
  auto comm = svtkm::cont::EnvironmentTracker::GetCommunicator();

  decltype(generator)::result_type seed = 0;
  if (comm.rank() == 0)
  {
    seed = static_cast<decltype(seed)>(std::time(nullptr));
    std::cout << "using seed: " << seed << "\n";
  }
  svtkmdiy::mpi::broadcast(comm, seed, 0);
  generator.seed(seed);

  return svtkm::cont::testing::Testing::Run(TestArrayHandleSerialization, argc, argv);
}

//-----------------------------------------------------------------------------
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<TestArrayHandleImplicit::ImplicitFunctor<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name =
      "TestArrayHandleImplicit::ImplicitFunctor<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

template <>
struct SerializableTypeString<TestArrayHandleTransform::TransformFunctor>
{
  static SVTKM_CONT const std::string Get() { return "TestArrayHandleTransform::TransformFunctor"; }
};

template <>
struct SerializableTypeString<TestArrayHandleTransform::InverseTransformFunctor>
{
  static SVTKM_CONT const std::string Get()
  {
    return "TestArrayHandleTransform::InverseTransformFunctor";
  }
};
}
} // svtkm::cont
