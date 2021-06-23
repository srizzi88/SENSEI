//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/ImageConnectivity.h>

#include <svtkm/cont/DataSet.h>

#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

svtkm::cont::DataSet MakeTestDataSet()
{
  // example from Figure 35.7 of Connected Component Labeling in CUDA by OndˇrejˇŚtava,
  // Bedˇrich Beneˇ
  std::vector<svtkm::UInt8> pixels{
    0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0,
    0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
  };

  svtkm::cont::DataSetBuilderUniform builder;
  svtkm::cont::DataSet dataSet = builder.Create(svtkm::Id3(8, 8, 1));

  svtkm::cont::DataSetFieldAdd dataSetFieldAdd;
  dataSetFieldAdd.AddPointField(dataSet, "color", pixels);

  return dataSet;
}

void TestImageConnectivity()
{
  svtkm::cont::DataSet dataSet = MakeTestDataSet();

  svtkm::filter::ImageConnectivity connectivity;
  connectivity.SetActiveField("color");

  const svtkm::cont::DataSet outputData = connectivity.Execute(dataSet);

  auto temp = outputData.GetField("component").GetData();
  svtkm::cont::ArrayHandle<svtkm::Id> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  std::vector<svtkm::Id> componentExpected = { 0, 1, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 1, 2,
                                              0, 1, 1, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 2,
                                              0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
                                              0, 1, 0, 1, 1, 1, 3, 3, 0, 1, 1, 1, 1, 1, 3, 3 };

  for (svtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
  {
    SVTKM_TEST_ASSERT(
      test_equal(resultArrayHandle.GetPortalConstControl().Get(i), componentExpected[size_t(i)]),
      "Wrong result for ImageConnectivity");
  }
}
}

int UnitTestImageConnectivityFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestImageConnectivity, argc, argv);
}
