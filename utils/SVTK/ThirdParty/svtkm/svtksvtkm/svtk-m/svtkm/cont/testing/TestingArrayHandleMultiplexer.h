//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingArrayHandleMultiplexer_h
#define svtk_m_cont_testing_TestingArrayHandleMultiplexer_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/ArrayHandleMultiplexer.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/cont/testing/Testing.h>

namespace svtkm
{
namespace cont
{
namespace testing
{

template <typename T>
struct TestValueFunctor
{
  SVTKM_EXEC_CONT T operator()(svtkm::Id index) const { return TestValue(index, T()); }
};

template <typename DeviceAdapter>
class TestingArrayHandleMultiplexer
{
  static constexpr svtkm::Id ARRAY_SIZE = 10;

  template <typename... Ts0, typename... Ts1>
  static void CheckArray(const svtkm::cont::ArrayHandleMultiplexer<Ts0...>& multiplexerArray,
                         const svtkm::cont::ArrayHandle<Ts1...>& expectedArray)
  {
    using T = typename std::remove_reference<decltype(multiplexerArray)>::type::ValueType;

    svtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
    SVTKM_TEST_ASSERT(test_equal_portals(multiplexerArray.GetPortalConstControl(),
                                        expectedArray.GetPortalConstControl()),
                     "Multiplexer array gave wrong result in control environment");

    svtkm::cont::ArrayHandle<T> copy;
    svtkm::cont::ArrayCopy(multiplexerArray, copy);
    SVTKM_TEST_ASSERT(
      test_equal_portals(copy.GetPortalConstControl(), expectedArray.GetPortalConstControl()),
      "Multiplexer did not copy correctly in execution environment");
  }

  static void BasicSwitch()
  {
    std::cout << std::endl << "--- Basic switch" << std::endl;

    using ValueType = svtkm::FloatDefault;

    using ArrayType1 = svtkm::cont::ArrayHandleConstant<ValueType>;
    ArrayType1 array1(TestValue(0, svtkm::FloatDefault{}), ARRAY_SIZE);

    using ArrayType2 = svtkm::cont::ArrayHandleCounting<ValueType>;
    ArrayType2 array2(TestValue(1, svtkm::FloatDefault{}), 1.0f, ARRAY_SIZE);

    auto array3 = svtkm::cont::make_ArrayHandleImplicit(TestValueFunctor<ValueType>{}, ARRAY_SIZE);
    using ArrayType3 = decltype(array3);

    svtkm::cont::ArrayHandleMultiplexer<ArrayType1, ArrayType2, ArrayType3> multiplexer;

    std::cout << "Check array1" << std::endl;
    multiplexer = array1;
    CheckArray(multiplexer, array1);

    std::cout << "Check array2" << std::endl;
    multiplexer = array2;
    CheckArray(multiplexer, array2);

    std::cout << "Check array3" << std::endl;
    multiplexer = array3;
    CheckArray(multiplexer, array3);
  }

  static void TestAll() { BasicSwitch(); }

public:
  static int Run(int argc, char* argv[])
  {
    svtkm::cont::ScopedRuntimeDeviceTracker device(DeviceAdapter{});
    return svtkm::cont::testing::Testing::Run(TestAll, argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif //svtk_m_cont_testing_TestingArrayHandleMultiplexer_h
