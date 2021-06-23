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
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>

#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>

#include <svtkm/Bitset.h>
#include <svtkm/Bounds.h>
#include <svtkm/Pair.h>
#include <svtkm/Range.h>

#include <svtkm/TypeList.h>
#include <svtkm/cont/testing/Testing.h>

#include <type_traits>

namespace
{

// clang-format off
template<typename T>
void is_noexcept_movable()
{
  constexpr bool valid = std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  SVTKM_TEST_ASSERT(valid, msg);
}

template<typename T>
void is_triv_noexcept_movable()
{
  constexpr bool valid =
#if !(defined(__GNUC__) && (__GNUC__ <= 5))
                         //GCC 4.X and compilers that act like it such as Intel 17.0
                         //don't have implementations for is_trivially_*
                         std::is_trivially_move_constructible<T>::value &&
                         std::is_trivially_move_assignable<T>::value &&
#endif
                         std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value &&
                         std::is_nothrow_constructible<T, T&&>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  SVTKM_TEST_ASSERT(valid, msg);
}
// clang-format on

struct IsTrivNoExcept
{
  template <typename T>
  void operator()(T) const
  {
    is_triv_noexcept_movable<T>();
  }
};

struct IsNoExceptHandle
{
  template <typename T>
  void operator()(T) const
  {
    using HandleType = svtkm::cont::ArrayHandle<T>;
    using VirtualType = svtkm::cont::ArrayHandleVirtual<T>;

    //verify the handle type
    is_noexcept_movable<HandleType>();
    is_noexcept_movable<VirtualType>();

    //verify the input portals of the handle
    is_noexcept_movable<decltype(
      std::declval<HandleType>().PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{}))>();
    is_noexcept_movable<decltype(
      std::declval<VirtualType>().PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{}))>();

    //verify the output portals of the handle
    is_noexcept_movable<decltype(
      std::declval<HandleType>().PrepareForOutput(2, svtkm::cont::DeviceAdapterTagSerial{}))>();
    is_noexcept_movable<decltype(
      std::declval<VirtualType>().PrepareForOutput(2, svtkm::cont::DeviceAdapterTagSerial{}))>();
  }
};

using svtkmComplexCustomTypes = svtkm::List<svtkm::Vec<svtkm::Vec<float, 3>, 3>,
                                          svtkm::Pair<svtkm::UInt64, svtkm::UInt64>,
                                          svtkm::Bitset<svtkm::UInt64>,
                                          svtkm::Bounds,
                                          svtkm::Range>;
}

//-----------------------------------------------------------------------------
void TestContDataTypesHaveMoveSemantics()
{
  //verify the Vec types are triv and noexcept
  svtkm::testing::Testing::TryTypes(IsTrivNoExcept{}, svtkm::TypeListVecCommon{});
  //verify that svtkm::Pair, Bitset, Bounds, and Range are triv and noexcept
  svtkm::testing::Testing::TryTypes(IsTrivNoExcept{}, svtkmComplexCustomTypes{});


  //verify that ArrayHandles and related portals are noexcept movable
  //allowing for efficient storage in containers such as std::vector
  svtkm::testing::Testing::TryTypes(IsNoExceptHandle{}, svtkm::TypeListAll{});

  svtkm::testing::Testing::TryTypes(IsNoExceptHandle{}, ::svtkmComplexCustomTypes{});

  //verify the DataSet, Field, CoordinateSystem, and ArrayHandleVirtualCoordinates
  //all have efficient storage in containers such as std::vector
  is_noexcept_movable<svtkm::cont::DataSet>();
  is_noexcept_movable<svtkm::cont::Field>();
  is_noexcept_movable<svtkm::cont::CoordinateSystem>();
  is_noexcept_movable<svtkm::cont::ArrayHandleVirtualCoordinates>();

  //verify the CellSetStructured, and CellSetExplicit
  //have efficient storage in containers such as std::vector
  is_noexcept_movable<svtkm::cont::CellSetStructured<2>>();
  is_noexcept_movable<svtkm::cont::CellSetStructured<3>>();
  is_noexcept_movable<svtkm::cont::CellSetExplicit<>>();
}


//-----------------------------------------------------------------------------
int UnitTestMoveConstructors(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestContDataTypesHaveMoveSemantics, argc, argv);
}
