//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/ImageMedian.h>

namespace
{

void TestImageMedian()
{
  std::cout << "Testing Image Median Filter on 3D structured data" << std::endl;

  svtkm::cont::testing::MakeTestDataSet testDataSet;
  svtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet2();

  svtkm::filter::ImageMedian median;
  median.Perform3x3();
  median.SetActiveField("pointvar");
  auto result = median.Execute(dataSet);

  SVTKM_TEST_ASSERT(result.HasPointField("median"), "Field missing.");
  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  result.GetPointField("median").GetData().CopyTo(resultArrayHandle);

  auto cells = result.GetCellSet().Cast<svtkm::cont::CellSetStructured<3>>();
  auto pdims = cells.GetPointDimensions();

  //verified by hand
  {
    auto portal = resultArrayHandle.GetPortalConstControl();
    std::cout << "spot to verify x = 1, y = 1, z = 0 is: ";
    svtkm::Float32 temp = portal.Get(1 + pdims[0]);
    std::cout << temp << std::endl << std::endl;
    SVTKM_TEST_ASSERT(test_equal(temp, 2), "incorrect median value");

    std::cout << "spot to verify x = 1, y = 1, z = 2 is: ";
    temp = portal.Get(1 + pdims[0] + (pdims[1] * pdims[0] * 2));
    std::cout << temp << std::endl << std::endl;
    SVTKM_TEST_ASSERT(test_equal(temp, 2.82843), "incorrect median value");
  }
}
}

int UnitTestImageMedianFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestImageMedian, argc, argv);
}
