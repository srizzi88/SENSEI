//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/ClipWithField.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

using Coord3D = svtkm::Vec3f;

svtkm::cont::DataSet MakeTestDatasetExplicit()
{
  std::vector<Coord3D> coords;
  coords.push_back(Coord3D(0.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 1.0f, 0.0f));
  coords.push_back(Coord3D(0.0f, 1.0f, 0.0f));

  std::vector<svtkm::Id> connectivity;
  connectivity.push_back(0);
  connectivity.push_back(1);
  connectivity.push_back(3);
  connectivity.push_back(3);
  connectivity.push_back(1);
  connectivity.push_back(2);

  svtkm::cont::DataSet ds;
  svtkm::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coords, svtkm::CellShapeTagTriangle(), 3, connectivity, "coords");

  std::vector<svtkm::Float32> values;
  values.push_back(1.0);
  values.push_back(2.0);
  values.push_back(1.0);
  values.push_back(0.0);
  svtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", values);

  return ds;
}

void TestClipExplicit()
{
  std::cout << "Testing Clip Filter on Explicit data" << std::endl;

  svtkm::cont::DataSet ds = MakeTestDatasetExplicit();

  svtkm::filter::ClipWithField clip;
  clip.SetClipValue(0.5);
  clip.SetActiveField("scalars");
  clip.SetFieldsToPass("scalars", svtkm::cont::Field::Association::POINTS);

  const svtkm::cont::DataSet outputData = clip.Execute(ds);

  SVTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");

  auto temp = outputData.GetField("scalars").GetData();
  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  svtkm::Float32 expected[7] = { 1, 2, 1, 0, 0.5, 0.5, 0.5 };
  for (int i = 0; i < 7; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for Clip fliter on triangle explicit data");
  }
}

// Adding for testing cases like Bug #329
// Other tests cover the specific cases of clipping, this test
// is to execute the clipping filter for a larger dataset.
// In this case the output is not verified against a sample.
void TestClipVolume()
{
  std::cout << "Testing Clip Filter on volumetric data" << std::endl;

  svtkm::Id3 dims(10, 10, 10);
  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::DataSet ds = maker.Make3DUniformDataSet3(dims);

  svtkm::filter::ClipWithField clip;
  clip.SetClipValue(0.0);
  clip.SetActiveField("pointvar");
  clip.SetFieldsToPass("pointvar", svtkm::cont::Field::Association::POINTS);

  const svtkm::cont::DataSet outputData = clip.Execute(ds);
}

void TestClip()
{
  //todo: add more clip tests
  TestClipExplicit();
  TestClipVolume();
}
}

int UnitTestClipWithFieldFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestClip, argc, argv);
}
