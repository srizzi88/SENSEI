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

#include <svtkm/filter/ExtractStructured.h>

using svtkm::cont::testing::MakeTestDataSet;

namespace
{

class TestingExtractStructured
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    svtkm::RangeId3 range(1, 4, 1, 4, 0, 1);
    svtkm::Id3 sample(1, 1, 1);

    svtkm::filter::ExtractStructured extract;
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 71.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(8) == 91.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 5.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(3) == 10.0f, "Wrong cell field data");
  }

  void TestUniform3D0() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // VOI within dataset
    extract.SetVOI(1, 4, 1, 4, 1, 4);
    extract.SetSampleRate(1, 1, 1);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 99.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(26) == 97.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 21.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(7) == 42.0f, "Wrong cell field data");
  }

  void TestUniform3D1() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // VOI surrounds dataset
    svtkm::Id3 minPoint(-1, -1, -1);
    svtkm::Id3 maxPoint(8, 8, 8);
    extract.SetVOI(minPoint, maxPoint);
    extract.SetSampleRate(1, 1, 1);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 125),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 64),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(31) == 99.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(93) == 97.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(63) == 63.0f, "Wrong cell field data");
  }

  void TestUniform3D2() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    svtkm::filter::ExtractStructured extract;

    // VOI surrounds dataset
    svtkm::RangeId3 range(-1, 3, -1, 3, -1, 3);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(26) == 15.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(7) == 21.0f, "Wrong cell field data");
  }

  void TestUniform3D3() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    svtkm::filter::ExtractStructured extract;

    // RangeId3 intersects dataset on far boundary
    svtkm::RangeId3 range(1, 8, 1, 8, 1, 8);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 64),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 27),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 99.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(63) == 0.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 21.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(26) == 63.0f, "Wrong cell field data");
  }

  void TestUniform3D4() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // RangeId3 intersects dataset without corner
    svtkm::RangeId3 range(2, 8, 1, 4, 1, 4);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 90.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(26) == 0.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 22.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(7) == 43.0f, "Wrong cell field data");
  }

  void TestUniform3D5() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // RangeId3 intersects dataset with plane
    svtkm::RangeId3 range(2, 8, 1, 2, 1, 4);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 90.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(8) == 0.0f, "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 22.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(3) == 39.0f, "Wrong cell field data");
  }

  void TestUniform3D6() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // RangeId3 within data set with sampling
    svtkm::RangeId3 range(0, 5, 0, 5, 1, 4);
    svtkm::Id3 sample(2, 2, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(26) == 0.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 16.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(3) == 26.0f, "Wrong cell field data");
  }

  void TestUniform3D7() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    svtkm::filter::ExtractStructured extract;

    // RangeId3 within data set with sampling
    svtkm::RangeId3 range(0, 5, 0, 5, 1, 4);
    svtkm::Id3 sample(3, 3, 2);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(7) == 97.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 16.0f, "Wrong cell field data");
  }

  void TestUniform3D8() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    svtkm::filter::ExtractStructured extract;

    // RangeId3 within data set with sampling
    svtkm::RangeId3 range(0, 5, 0, 5, 1, 4);
    svtkm::Id3 sample(3, 3, 2);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);
    extract.SetIncludeBoundary(true);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 18),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(4) == 99.0f,
                     "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(13) == 97.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 16.0f, "Wrong cell field data");
    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(3) == 31.0f, "Wrong cell field data");
  }

  void TestRectilinear2D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DRectilinearDataSet0();

    svtkm::filter::ExtractStructured extract;

    // RangeId3
    svtkm::RangeId3 range(0, 2, 0, 2, 0, 1);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 4),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(3) == 4.0f, "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.0f, "Wrong cell field data");
  }

  void TestRectilinear3D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DRectilinearDataSet0();

    svtkm::filter::ExtractStructured extract;

    // RangeId3 and subsample
    svtkm::RangeId3 range(0, 2, 0, 2, 0, 2);
    svtkm::Id3 sample(1, 1, 1);
    extract.SetVOI(range);
    extract.SetSampleRate(sample);

    extract.SetFieldsToPass({ "pointvar", "cellvar" });
    svtkm::cont::DataSet output = extract.Execute(dataset);
    SVTKM_TEST_ASSERT(test_equal(output.GetCellSet().GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    svtkm::cont::ArrayHandle<svtkm::Float32> outPointData;
    svtkm::cont::ArrayHandle<svtkm::Float32> outCellData;
    output.GetField("pointvar").GetData().CopyTo(outPointData);
    output.GetField("cellvar").GetData().CopyTo(outCellData);

    SVTKM_TEST_ASSERT(
      test_equal(output.GetCellSet().GetNumberOfPoints(), outPointData.GetNumberOfValues()),
      "Data/Geometry mismatch for ExtractStructured filter");
    SVTKM_TEST_ASSERT(test_equal(output.GetNumberOfCells(), outCellData.GetNumberOfValues()),
                     "Data/Geometry mismatch for ExtractStructured filter");

    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(0) == 0.0f, "Wrong point field data");
    SVTKM_TEST_ASSERT(outPointData.GetPortalConstControl().Get(7) == 10.0f,
                     "Wrong point field data");

    SVTKM_TEST_ASSERT(outCellData.GetPortalConstControl().Get(0) == 0.0f, "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D0();
    this->TestUniform3D1();
    this->TestUniform3D2();
    this->TestUniform3D3();
    this->TestUniform3D4();
    this->TestUniform3D5();
    this->TestUniform3D6();
    this->TestUniform3D7();
    this->TestUniform3D8();
    this->TestRectilinear2D();
    this->TestRectilinear3D();
  }
};
}

int UnitTestExtractStructuredFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingExtractStructured(), argc, argv);
}
