//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/Magnitude.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

void TestMagnitude()
{
  std::cout << "Testing Magnitude Worklet" << std::endl;

  svtkm::worklet::Magnitude magnitudeWorklet;

  using ArrayReturnType = svtkm::cont::ArrayHandle<svtkm::Float64>;
  using ArrayVectorType = svtkm::cont::ArrayHandle<svtkm::Vec4i_32>;
  using PortalType = ArrayVectorType::PortalControl;

  ArrayVectorType pythagoreanTriples;
  pythagoreanTriples.Allocate(5);
  PortalType pt = pythagoreanTriples.GetPortalControl();

  pt.Set(0, svtkm::make_Vec(3, 4, 5, 0));
  pt.Set(1, svtkm::make_Vec(5, 12, 13, 0));
  pt.Set(2, svtkm::make_Vec(8, 15, 17, 0));
  pt.Set(3, svtkm::make_Vec(7, 24, 25, 0));
  pt.Set(4, svtkm::make_Vec(9, 40, 41, 0));

  svtkm::worklet::DispatcherMapField<svtkm::worklet::Magnitude> dispatcher(magnitudeWorklet);

  ArrayReturnType result;

  dispatcher.Invoke(pythagoreanTriples, result);

  for (svtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(
      test_equal(std::sqrt(pt.Get(i)[0] * pt.Get(i)[0] + pt.Get(i)[1] * pt.Get(i)[1] +
                           pt.Get(i)[2] * pt.Get(i)[2]),
                 result.GetPortalConstControl().Get(i)),
      "Wrong result for Magnitude worklet");
  }
}
}

int UnitTestMagnitude(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestMagnitude, argc, argv);
}
