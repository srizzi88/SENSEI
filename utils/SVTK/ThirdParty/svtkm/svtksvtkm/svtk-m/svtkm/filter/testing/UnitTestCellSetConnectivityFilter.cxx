//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/CellSetConnectivity.h>
#include <svtkm/filter/Contour.h>
#include <svtkm/source/Tangle.h>

namespace
{

class TestCellSetConnectivity
{
public:
  void TestTangleIsosurface() const
  {
    svtkm::Id3 dims(4, 4, 4);
    svtkm::source::Tangle tangle(dims);
    svtkm::cont::DataSet dataSet = tangle.Execute();

    svtkm::filter::Contour filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("nodevar");
    svtkm::cont::DataSet iso = filter.Execute(dataSet);

    svtkm::filter::CellSetConnectivity connectivity;
    const svtkm::cont::DataSet output = connectivity.Execute(iso);

    svtkm::cont::ArrayHandle<svtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.CopyTo(componentArray);

    using Algorithm = svtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    SVTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                     "Wrong number of connected components");
  }

  void TestExplicitDataSet() const
  {
    svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

    svtkm::filter::CellSetConnectivity connectivity;
    const svtkm::cont::DataSet output = connectivity.Execute(dataSet);

    svtkm::cont::ArrayHandle<svtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.CopyTo(componentArray);

    using Algorithm = svtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    SVTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void TestUniformDataSet() const
  {
    svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
    svtkm::filter::CellSetConnectivity connectivity;
    const svtkm::cont::DataSet output = connectivity.Execute(dataSet);

    svtkm::cont::ArrayHandle<svtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.CopyTo(componentArray);

    using Algorithm = svtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    SVTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void operator()() const
  {
    this->TestTangleIsosurface();
    this->TestExplicitDataSet();
    this->TestUniformDataSet();
  }
};
}

int UnitTestCellSetConnectivityFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
