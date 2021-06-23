//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleTransform.h>

#include <svtkm/VecTraits.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

struct MySquare
{
  template <typename U>
  SVTKM_EXEC auto operator()(U u) const -> decltype(svtkm::Dot(u, u))
  {
    return svtkm::Dot(u, u);
  }
};

template <typename OriginalPortalType, typename TransformedPortalType>
struct CheckTransformFunctor : svtkm::exec::FunctorBase
{
  OriginalPortalType OriginalPortal;
  TransformedPortalType TransformedPortal;

  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    using T = typename TransformedPortalType::ValueType;
    typename OriginalPortalType::ValueType original = this->OriginalPortal.Get(index);
    T transformed = this->TransformedPortal.Get(index);
    if (!test_equal(transformed, MySquare{}(original)))
    {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType, typename Device>
SVTKM_CONT CheckTransformFunctor<
  typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst,
  typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst>
make_CheckTransformFunctor(const OriginalArrayHandleType& originalArray,
                           const TransformedArrayHandleType& transformedArray,
                           Device)
{
  using OriginalPortalType =
    typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  using TransformedPortalType =
    typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst;
  CheckTransformFunctor<OriginalPortalType, TransformedPortalType> functor;
  functor.OriginalPortal = originalArray.PrepareForInput(Device());
  functor.TransformedPortal = transformedArray.PrepareForInput(Device());
  return functor;
}

template <typename OriginalArrayHandleType, typename TransformedArrayHandleType>
SVTKM_CONT void CheckControlPortals(const OriginalArrayHandleType& originalArray,
                                   const TransformedArrayHandleType& transformedArray)
{
  std::cout << "  Verify that the control portal works" << std::endl;

  using OriginalPortalType = typename OriginalArrayHandleType::PortalConstControl;
  using TransformedPortalType = typename TransformedArrayHandleType::PortalConstControl;

  SVTKM_TEST_ASSERT(originalArray.GetNumberOfValues() == transformedArray.GetNumberOfValues(),
                   "Number of values in transformed array incorrect.");

  OriginalPortalType originalPortal = originalArray.GetPortalConstControl();
  TransformedPortalType transformedPortal = transformedArray.GetPortalConstControl();

  SVTKM_TEST_ASSERT(originalPortal.GetNumberOfValues() == transformedPortal.GetNumberOfValues(),
                   "Number of values in transformed portal incorrect.");

  for (svtkm::Id index = 0; index < originalArray.GetNumberOfValues(); index++)
  {
    using T = typename TransformedPortalType::ValueType;
    typename OriginalPortalType::ValueType original = originalPortal.Get(index);
    T transformed = transformedPortal.Get(index);
    SVTKM_TEST_ASSERT(test_equal(transformed, MySquare{}(original)), "Bad transform value.");
  }
}

template <typename InputValueType>
struct TransformTests
{
  using OutputValueType = typename svtkm::VecTraits<InputValueType>::ComponentType;

  using TransformHandle =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandle<InputValueType>, MySquare>;

  using CountingTransformHandle =
    svtkm::cont::ArrayHandleTransform<svtkm::cont::ArrayHandleCounting<InputValueType>, MySquare>;

  using Device = svtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = svtkm::cont::DeviceAdapterAlgorithm<Device>;

  void operator()() const
  {
    MySquare functor;

    std::cout << "Test a transform handle with a counting handle as the values" << std::endl;
    svtkm::cont::ArrayHandleCounting<InputValueType> counting = svtkm::cont::make_ArrayHandleCounting(
      InputValueType(OutputValueType(0)), InputValueType(1), ARRAY_SIZE);
    CountingTransformHandle countingTransformed =
      svtkm::cont::make_ArrayHandleTransform(counting, functor);

    CheckControlPortals(counting, countingTransformed);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(counting, countingTransformed, Device()),
                        ARRAY_SIZE);

    std::cout << "Test a transform handle with a normal handle as the values" << std::endl;
    //we are going to connect the two handles up, and than fill
    //the values and make the transform sees the new values in the handle
    svtkm::cont::ArrayHandle<InputValueType> input;
    TransformHandle thandle(input, functor);

    using Portal = typename svtkm::cont::ArrayHandle<InputValueType>::PortalControl;
    input.Allocate(ARRAY_SIZE);
    Portal portal = input.GetPortalControl();
    for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, TestValue(index, InputValueType()));
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(input, thandle, Device()), ARRAY_SIZE);

    std::cout << "Modify array handle values to ensure transform gets updated" << std::endl;
    for (svtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, TestValue(index * index, InputValueType()));
    }

    CheckControlPortals(input, thandle);

    std::cout << "  Verify that the execution portal works" << std::endl;
    Algorithm::Schedule(make_CheckTransformFunctor(input, thandle, Device()), ARRAY_SIZE);
  }
};

struct TryInputType
{
  template <typename InputType>
  void operator()(InputType) const
  {
    TransformTests<InputType>()();
  }
};

void TestArrayHandleTransform()
{
  svtkm::testing::Testing::TryTypes(TryInputType());
}

} // anonymous namespace

int UnitTestArrayHandleTransform(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayHandleTransform, argc, argv);
}
