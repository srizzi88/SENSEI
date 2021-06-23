//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/ClipWithImplicitFunction.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

using Coord3D = svtkm::Vec3f;

svtkm::cont::DataSet MakeTestDatasetStructured()
{
  static constexpr svtkm::Id xdim = 3, ydim = 3;
  static const svtkm::Id2 dim(xdim, ydim);
  static constexpr svtkm::Id numVerts = xdim * ydim;

  svtkm::Float32 scalars[numVerts];
  for (svtkm::Id i = 0; i < numVerts; ++i)
  {
    scalars[i] = 1.0f;
  }
  scalars[4] = 0.0f;

  svtkm::cont::DataSet ds;
  svtkm::cont::DataSetBuilderUniform builder;
  ds = builder.Create(dim);

  svtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", scalars, numVerts);

  return ds;
}

void TestClipStructured()
{
  std::cout << "Testing ClipWithImplicitFunction Filter on Structured data" << std::endl;

  svtkm::cont::DataSet ds = MakeTestDatasetStructured();

  svtkm::Vec3f center(1, 1, 0);
  svtkm::FloatDefault radius(0.5);

  svtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(svtkm::cont::make_ImplicitFunctionHandle(svtkm::Sphere(center, radius)));
  clip.SetFieldsToPass("scalars");

  svtkm::cont::DataSet outputData = clip.Execute(ds);

  SVTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 8,
                   "Wrong number of cells in the output dataset");

  svtkm::cont::VariantArrayHandle temp = outputData.GetField("scalars").GetData();
  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  SVTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  svtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClipStructuredInverted()
{
  std::cout << "Testing ClipWithImplicitFunctionInverted Filter on Structured data" << std::endl;

  svtkm::cont::DataSet ds = MakeTestDatasetStructured();

  svtkm::Vec3f center(1, 1, 0);
  svtkm::FloatDefault radius(0.5);

  svtkm::filter::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(svtkm::cont::make_ImplicitFunctionHandle(svtkm::Sphere(center, radius)));
  bool invert = true;
  clip.SetInvertClip(invert);
  clip.SetFieldsToPass("scalars");
  auto outputData = clip.Execute(ds);

  SVTKM_TEST_ASSERT(outputData.GetNumberOfFields() == 1,
                   "Wrong number of fields in the output dataset");
  SVTKM_TEST_ASSERT(outputData.GetNumberOfCells() == 4,
                   "Wrong number of cells in the output dataset");

  svtkm::cont::VariantArrayHandle temp = outputData.GetField("scalars").GetData();
  svtkm::cont::ArrayHandle<svtkm::Float32> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);

  SVTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == 13,
                   "Wrong number of points in the output dataset");

  svtkm::Float32 expected[13] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  for (int i = 0; i < 13; ++i)
  {
    SVTKM_TEST_ASSERT(test_equal(resultArrayHandle.GetPortalConstControl().Get(i), expected[i]),
                     "Wrong result for ClipWithImplicitFunction fliter on sturctured quads data");
  }
}

void TestClip()
{
  //todo: add more clip tests
  TestClipStructured();
  TestClipStructuredInverted();
}

} // anonymous namespace

int UnitTestClipWithImplicitFunctionFilter(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestClip, argc, argv);
}
