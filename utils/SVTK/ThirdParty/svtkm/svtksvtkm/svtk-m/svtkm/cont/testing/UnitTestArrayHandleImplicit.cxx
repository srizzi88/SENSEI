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
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/VecTraits.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  SVTKM_EXEC_CONT
  ValueType operator()(svtkm::Id i) const
  {
    using ComponentType = typename svtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(i * i));
  }
};

struct ImplicitTests
{
  template <typename ValueType>
  void operator()(const ValueType) const
  {
    using FunctorType = IndexSquared<ValueType>;
    FunctorType functor;

    using ImplicitHandle = svtkm::cont::ArrayHandleImplicit<FunctorType>;

    ImplicitHandle implict = svtkm::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

    //verify that the control portal works
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = implict.GetPortalConstControl().Get(i);
      const ValueType correct_value = functor(i);
      SVTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    //verify that the execution portal works
    using Device = svtkm::cont::DeviceAdapterTagSerial;
    using CEPortal = typename ImplicitHandle::template ExecutionTypes<Device>::PortalConst;
    CEPortal execPortal = implict.PrepareForInput(Device());
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = execPortal.Get(i);
      const ValueType correct_value = functor(i);
      SVTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }
  }
};

void TestArrayHandleImplicit()
{
  svtkm::testing::Testing::TryTypes(ImplicitTests(), svtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestArrayHandleImplicit(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayHandleImplicit, argc, argv);
}
