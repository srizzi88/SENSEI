//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleSwizzle.h>

#include <svtkm/cont/testing/Testing.h>

#include <type_traits>

namespace
{

template <typename ValueType>
struct SwizzleTests
{
  using SwizzleInputArrayType = svtkm::cont::ArrayHandle<svtkm::Vec<ValueType, 4>>;

  template <svtkm::IdComponent OutSize>
  using SwizzleArrayType = svtkm::cont::ArrayHandleSwizzle<SwizzleInputArrayType, OutSize>;

  using ReferenceComponentArrayType = svtkm::cont::ArrayHandleCounting<ValueType>;
  using ReferenceArrayType = svtkm::cont::ArrayHandleCompositeVector<ReferenceComponentArrayType,
                                                                    ReferenceComponentArrayType,
                                                                    ReferenceComponentArrayType,
                                                                    ReferenceComponentArrayType>;

  template <svtkm::IdComponent Size>
  using MapType = svtkm::Vec<svtkm::IdComponent, Size>;

  using Algo = svtkm::cont::Algorithm;

  // This is used to build a ArrayHandleSwizzle's internal array.
  ReferenceArrayType RefArray;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const svtkm::Id numValues = 32;
    ReferenceComponentArrayType c1 =
      svtkm::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArrayType c2 =
      svtkm::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArrayType c3 =
      svtkm::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArrayType c4 =
      svtkm::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefArray = svtkm::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4);
  }

  SwizzleInputArrayType BuildSwizzleInputArray() const
  {
    SwizzleInputArrayType result;
    Algo::Copy(this->RefArray, result);
    return result;
  }

  template <svtkm::IdComponent OutSize>
  void SanityCheck(const MapType<OutSize>& map) const
  {
    using Swizzle = SwizzleArrayType<OutSize>;
    using Traits = typename Swizzle::SwizzleTraits;

    SVTKM_TEST_ASSERT(Traits::OutVecSize ==
                       svtkm::VecTraits<typename Swizzle::ValueType>::NUM_COMPONENTS,
                     "Traits::OutVecSize invalid.");
    SVTKM_TEST_ASSERT(
      SVTKM_PASS_COMMAS(std::is_same<typename Traits::ComponentType, ValueType>::value),
      "Traits::ComponentType invalid.");
    SVTKM_TEST_ASSERT(
      SVTKM_PASS_COMMAS(
        std::is_same<typename Traits::OutValueType, svtkm::Vec<ValueType, OutSize>>::value),
      "Traits::OutValueType invalid.");

    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    auto swizzle = svtkm::cont::make_ArrayHandleSwizzle(input, map);

    SVTKM_TEST_ASSERT(input.GetNumberOfValues() == swizzle.GetNumberOfValues(),
                     "Number of values in copied Swizzle array does not match input.");
  }

  template <svtkm::IdComponent OutSize>
  void ReadTest(const MapType<OutSize>& map) const
  {
    using Traits = typename SwizzleArrayType<OutSize>::SwizzleTraits;

    // Test that the expected values are read from an Swizzle array.
    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    auto swizzle = svtkm::cont::make_ArrayHandleSwizzle(input, map);

    // Test reading the data back in the control env:
    this->ValidateReadTest(swizzle, map);

    // Copy the extracted array in the execution environment to test reading:
    svtkm::cont::ArrayHandle<typename Traits::OutValueType> execCopy;
    Algo::Copy(swizzle, execCopy);
    this->ValidateReadTest(execCopy, map);
  }

  template <typename ArrayHandleType, svtkm::IdComponent OutSize>
  void ValidateReadTest(ArrayHandleType testArray, const MapType<OutSize>& map) const
  {
    using Traits = typename SwizzleArrayType<OutSize>::SwizzleTraits;
    using ReferenceVectorType = typename ReferenceArrayType::ValueType;
    using SwizzleVectorType = typename Traits::OutValueType;

    SVTKM_TEST_ASSERT(map.GetNumberOfComponents() ==
                       svtkm::VecTraits<SwizzleVectorType>::NUM_COMPONENTS,
                     "Unexpected runtime component map size.");
    SVTKM_TEST_ASSERT(testArray.GetNumberOfValues() == this->RefArray.GetNumberOfValues(),
                     "Number of values incorrect in Read test.");

    auto refPortal = this->RefArray.GetPortalConstControl();
    auto testPortal = testArray.GetPortalConstControl();

    SwizzleVectorType refVecSwizzle(svtkm::TypeTraits<SwizzleVectorType>::ZeroInitialization());
    for (svtkm::Id i = 0; i < testArray.GetNumberOfValues(); ++i)
    {
      ReferenceVectorType refVec = refPortal.Get(i);

      // Manually swizzle the reference vector using the runtime map information:
      for (svtkm::IdComponent j = 0; j < map.GetNumberOfComponents(); ++j)
      {
        refVecSwizzle[j] = refVec[map[j]];
      }

      SVTKM_TEST_ASSERT(test_equal(refVecSwizzle, testPortal.Get(i), 0.),
                       "Invalid value encountered in Read test.");
    }
  }

  // Doubles everything in the input portal.
  template <typename PortalType>
  struct WriteTestFunctor : svtkm::exec::FunctorBase
  {
    PortalType Portal;

    SVTKM_CONT
    WriteTestFunctor(const PortalType& portal)
      : Portal(portal)
    {
    }

    SVTKM_EXEC_CONT
    void operator()(svtkm::Id index) const { this->Portal.Set(index, this->Portal.Get(index) * 2.); }
  };

  struct WriteExec
  {
    template <typename DeviceTag, typename SwizzleHandleType>
    bool operator()(DeviceTag, SwizzleHandleType& swizzle) const
    {
      using Portal = typename SwizzleHandleType::template ExecutionTypes<DeviceTag>::Portal;
      WriteTestFunctor<Portal> functor(swizzle.PrepareForInPlace(DeviceTag()));
      Algo::Schedule(functor, swizzle.GetNumberOfValues());
      return true;
    }
  };


  template <svtkm::IdComponent OutSize>
  void WriteTest(const MapType<OutSize>& map) const
  {
    // Control test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      auto swizzle = svtkm::cont::make_ArrayHandleSwizzle(input, map);

      WriteTestFunctor<typename SwizzleArrayType<OutSize>::PortalControl> functor(
        swizzle.GetPortalControl());

      for (svtkm::Id i = 0; i < swizzle.GetNumberOfValues(); ++i)
      {
        functor(i);
      }

      this->ValidateWriteTestArray(input, map);
    }

    // Exec test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      auto swizzle = svtkm::cont::make_ArrayHandleSwizzle(input, map);

      svtkm::cont::TryExecute(WriteExec{}, swizzle);
      this->ValidateWriteTestArray(input, map);
    }
  }

  // Check that the swizzled components are twice the reference value.
  template <svtkm::IdComponent OutSize>
  void ValidateWriteTestArray(SwizzleInputArrayType testArray, const MapType<OutSize>& map) const
  {
    auto refPortal = this->RefArray.GetPortalConstControl();
    auto portal = testArray.GetPortalConstControl();

    SVTKM_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in write test output do not match input.");

    for (svtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);

      // Double all of the components that appear in the map to replicate the
      // test result:
      for (svtkm::IdComponent j = 0; j < map.GetNumberOfComponents(); ++j)
      {
        refValue[map[j]] *= 2;
      }

      SVTKM_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in Write test.");
    }
  }

  template <svtkm::IdComponent OutSize>
  void TestSwizzle(const MapType<OutSize>& map) const
  {
    this->SanityCheck(map);
    this->ReadTest(map);

    this->WriteTest(map);
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestSwizzle(svtkm::make_Vec(0, 1));
    this->TestSwizzle(svtkm::make_Vec(0, 2));
    this->TestSwizzle(svtkm::make_Vec(0, 3));
    this->TestSwizzle(svtkm::make_Vec(1, 0));
    this->TestSwizzle(svtkm::make_Vec(1, 2));
    this->TestSwizzle(svtkm::make_Vec(1, 3));
    this->TestSwizzle(svtkm::make_Vec(2, 0));
    this->TestSwizzle(svtkm::make_Vec(2, 1));
    this->TestSwizzle(svtkm::make_Vec(2, 3));
    this->TestSwizzle(svtkm::make_Vec(3, 0));
    this->TestSwizzle(svtkm::make_Vec(3, 1));
    this->TestSwizzle(svtkm::make_Vec(3, 2));
    this->TestSwizzle(svtkm::make_Vec(0, 1, 2));
    this->TestSwizzle(svtkm::make_Vec(0, 1, 3));
    this->TestSwizzle(svtkm::make_Vec(0, 2, 1));
    this->TestSwizzle(svtkm::make_Vec(0, 2, 3));
    this->TestSwizzle(svtkm::make_Vec(0, 3, 1));
    this->TestSwizzle(svtkm::make_Vec(0, 3, 2));
    this->TestSwizzle(svtkm::make_Vec(1, 0, 2));
    this->TestSwizzle(svtkm::make_Vec(1, 0, 3));
    this->TestSwizzle(svtkm::make_Vec(1, 2, 0));
    this->TestSwizzle(svtkm::make_Vec(1, 2, 3));
    this->TestSwizzle(svtkm::make_Vec(1, 3, 0));
    this->TestSwizzle(svtkm::make_Vec(1, 3, 2));
    this->TestSwizzle(svtkm::make_Vec(2, 0, 1));
    this->TestSwizzle(svtkm::make_Vec(2, 0, 3));
    this->TestSwizzle(svtkm::make_Vec(2, 1, 0));
    this->TestSwizzle(svtkm::make_Vec(2, 1, 3));
    this->TestSwizzle(svtkm::make_Vec(2, 3, 0));
    this->TestSwizzle(svtkm::make_Vec(2, 3, 1));
    this->TestSwizzle(svtkm::make_Vec(3, 0, 1));
    this->TestSwizzle(svtkm::make_Vec(3, 0, 2));
    this->TestSwizzle(svtkm::make_Vec(3, 1, 0));
    this->TestSwizzle(svtkm::make_Vec(3, 1, 2));
    this->TestSwizzle(svtkm::make_Vec(3, 2, 0));
    this->TestSwizzle(svtkm::make_Vec(3, 2, 1));
    this->TestSwizzle(svtkm::make_Vec(0, 1, 2, 3));
    this->TestSwizzle(svtkm::make_Vec(0, 1, 3, 2));
    this->TestSwizzle(svtkm::make_Vec(0, 2, 1, 3));
    this->TestSwizzle(svtkm::make_Vec(0, 2, 3, 1));
    this->TestSwizzle(svtkm::make_Vec(0, 3, 1, 2));
    this->TestSwizzle(svtkm::make_Vec(0, 3, 2, 1));
    this->TestSwizzle(svtkm::make_Vec(1, 0, 2, 3));
    this->TestSwizzle(svtkm::make_Vec(1, 0, 3, 2));
    this->TestSwizzle(svtkm::make_Vec(1, 2, 0, 3));
    this->TestSwizzle(svtkm::make_Vec(1, 2, 3, 0));
    this->TestSwizzle(svtkm::make_Vec(1, 3, 0, 2));
    this->TestSwizzle(svtkm::make_Vec(1, 3, 2, 0));
    this->TestSwizzle(svtkm::make_Vec(2, 0, 1, 3));
    this->TestSwizzle(svtkm::make_Vec(2, 0, 3, 1));
    this->TestSwizzle(svtkm::make_Vec(2, 1, 0, 3));
    this->TestSwizzle(svtkm::make_Vec(2, 1, 3, 0));
    this->TestSwizzle(svtkm::make_Vec(2, 3, 0, 1));
    this->TestSwizzle(svtkm::make_Vec(2, 3, 1, 0));
    this->TestSwizzle(svtkm::make_Vec(3, 0, 1, 2));
    this->TestSwizzle(svtkm::make_Vec(3, 0, 2, 1));
    this->TestSwizzle(svtkm::make_Vec(3, 1, 0, 2));
    this->TestSwizzle(svtkm::make_Vec(3, 1, 2, 0));
    this->TestSwizzle(svtkm::make_Vec(3, 2, 0, 1));
    this->TestSwizzle(svtkm::make_Vec(3, 2, 1, 0));
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    SwizzleTests<ValueType>()();
  }
};

void TestArrayHandleSwizzle()
{
  using TestTypes = svtkm::List<svtkm::Int32, svtkm::Int64, svtkm::Float32, svtkm::Float64>;
  svtkm::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

void TestComponentMapValidator()
{
  svtkm::cont::ArrayHandle<svtkm::Id4> dummy;

  // Repeat components:
  bool error = false;
  try
  {
    svtkm::cont::make_ArrayHandleSwizzle(dummy, 0, 1, 2, 1);
    error = true;
  }
  catch (svtkm::cont::ErrorBadValue& e)
  {
    std::cout << "Caught expected exception 1: " << e.what() << "\n";
  }
  SVTKM_TEST_ASSERT(!error, "Repeat components allowed.");

  try
  {
    svtkm::cont::make_ArrayHandleSwizzle(dummy, 0, 1, 2, -1);
    error = true;
  }
  catch (svtkm::cont::ErrorBadValue& e)
  {
    std::cout << "Caught expected exception 2: " << e.what() << "\n";
  }
  SVTKM_TEST_ASSERT(!error, "Negative components allowed.");

  try
  {
    svtkm::cont::make_ArrayHandleSwizzle(dummy, 0, 1, 2, 5);
    error = true;
  }
  catch (svtkm::cont::ErrorBadValue& e)
  {
    std::cout << "Caught expected exception 3: " << e.what() << "\n";
  }
  SVTKM_TEST_ASSERT(!error, "Invalid component allowed.");
}

} // end anon namespace

int UnitTestArrayHandleSwizzle(int argc, char* argv[])
{
  try
  {
    TestComponentMapValidator();
  }
  catch (svtkm::cont::Error& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return svtkm::cont::testing::Testing::Run(TestArrayHandleSwizzle, argc, argv);
}
