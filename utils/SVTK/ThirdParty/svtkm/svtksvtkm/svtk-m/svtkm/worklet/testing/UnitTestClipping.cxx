//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/Clip.h>

#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/cont/testing/Testing.h>

#include <vector>

using Coord3D = svtkm::Vec3f;

const svtkm::Float32 clipValue = 0.5;

template <typename T, typename Storage>
bool TestArrayHandle(const svtkm::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     svtkm::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  for (svtkm::Id i = 0; i < size; ++i)
  {
    if (ah.GetPortalConstControl().Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

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

  svtkm::cont::DataSetFieldAdd fieldAdder;

  std::vector<svtkm::Float32> values;
  values.push_back(1.0);
  values.push_back(2.0);
  values.push_back(1.0);
  values.push_back(0.0);
  fieldAdder.AddPointField(ds, "scalars", values);

  values.clear();
  values.push_back(100.f);
  values.push_back(-100.f);
  fieldAdder.AddCellField(ds, "cellvar", values);

  return ds;
}

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

  std::vector<svtkm::Float32> cellvar = { -100.f, 100.f, 30.f, -30.f };
  fieldAdder.AddCellField(ds, "cellvar", cellvar);

  return ds;
}

void TestClippingExplicit()
{
  svtkm::cont::DataSet ds = MakeTestDatasetExplicit();
  svtkm::worklet::Clip clip;
  bool invertClip = false;
  svtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(),
             ds.GetField("scalars").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
             clipValue,
             invertClip);

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  svtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> scalars = clip.ProcessPointField(scalarsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> cellvar = clip.ProcessCellField(cellvarIn);

  svtkm::Id connectivitySize = 8;
  svtkm::Id fieldSize = 7;
  svtkm::Id expectedConnectivity[] = { 0, 1, 5, 4, 1, 2, 6, 5 };
  Coord3D expectedCoords[] = {
    Coord3D(0.00f, 0.00f, 0.0f), Coord3D(1.00f, 0.00f, 0.0f), Coord3D(1.00f, 1.00f, 0.0f),
    Coord3D(0.00f, 1.00f, 0.0f), Coord3D(0.00f, 0.50f, 0.0f), Coord3D(0.25f, 0.75f, 0.0f),
    Coord3D(0.50f, 1.00f, 0.0f),
  };
  svtkm::Float32 expectedScalars[] = { 1, 2, 1, 0, 0.5, 0.5, 0.5 };
  std::vector<svtkm::Float32> expectedCellvar = { 100.f, -100.f };

  SVTKM_TEST_ASSERT(outputCellSet.GetNumberOfPoints() == fieldSize,
                   "Wrong number of points in cell set.");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                       svtkm::TopologyElementTagPoint()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  SVTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  SVTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<svtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

void TestClippingStructured()
{
  using CoordsValueType = svtkm::cont::ArrayHandleUniformPointCoordinates::ValueType;
  using CoordsOutType = svtkm::cont::ArrayHandle<CoordsValueType>;

  svtkm::cont::DataSet ds = MakeTestDatasetStructured();

  bool invertClip = false;
  svtkm::worklet::Clip clip;
  svtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(),
             ds.GetField("scalars").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
             clipValue,
             invertClip);

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  CoordsOutType coords = clip.ProcessPointField(coordsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> scalars = clip.ProcessPointField(scalarsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> cellvar = clip.ProcessCellField(cellvarIn);


  svtkm::Id connectivitySize = 28;
  svtkm::Id fieldSize = 13;
  svtkm::Id expectedConnectivity[] = { 9,  10, 3, 1, 1, 3, 0, 11, 9,  1, 5, 5, 1, 2,
                                      10, 12, 7, 3, 3, 7, 6, 12, 11, 5, 7, 7, 5, 8 };

  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f), Coord3D(1.0f, 0.0f, 0.0f), Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f), Coord3D(1.0f, 1.0f, 0.0f), Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f), Coord3D(1.0f, 2.0f, 0.0f), Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.5f, 0.0f), Coord3D(0.5f, 1.0f, 0.0f), Coord3D(1.5f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.5f, 0.0f),
  };
  svtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.5, 0.5, 0.5, 0.5 };
  std::vector<svtkm::Float32> expectedCellvar = { -100.f, -100.f, 100.f, 100.f,
                                                 30.f,   30.f,   -30.f, -30.f };

  SVTKM_TEST_ASSERT(outputCellSet.GetNumberOfPoints() == fieldSize,
                   "Wrong number of points in cell set.");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                       svtkm::TopologyElementTagPoint()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  SVTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  SVTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<svtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

void TestClippingWithImplicitFunction()
{
  svtkm::Vec3f center(1, 1, 0);
  svtkm::FloatDefault radius(0.5);

  svtkm::cont::DataSet ds = MakeTestDatasetStructured();

  bool invertClip = false;
  svtkm::worklet::Clip clip;
  svtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(),
             svtkm::cont::make_ImplicitFunctionHandle<svtkm::Sphere>(center, radius),
             ds.GetCoordinateSystem("coords"),
             invertClip);

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  svtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> scalars = clip.ProcessPointField(scalarsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> cellvar = clip.ProcessCellField(cellvarIn);

  svtkm::Id connectivitySize = 28;
  svtkm::Id fieldSize = 13;

  svtkm::Id expectedConnectivity[] = { 9,  10, 3, 1, 1, 3, 0, 11, 9,  1, 5, 5, 1, 2,
                                      10, 12, 7, 3, 3, 7, 6, 12, 11, 5, 7, 7, 5, 8 };

  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f),  Coord3D(1.0f, 0.0f, 0.0f),  Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f),  Coord3D(1.0f, 1.0f, 0.0f),  Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f),  Coord3D(1.0f, 2.0f, 0.0f),  Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.75f, 0.0f), Coord3D(0.75f, 1.0f, 0.0f), Coord3D(1.25f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.25f, 0.0f),
  };
  svtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  std::vector<svtkm::Float32> expectedCellvar = { -100.f, -100.f, 100.f, 100.f,
                                                 30.f,   30.f,   -30.f, -30.f };

  SVTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                       svtkm::TopologyElementTagPoint()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  SVTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  SVTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<svtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

void TestClippingWithImplicitFunctionInverted()
{
  svtkm::Vec3f center(1, 1, 0);
  svtkm::FloatDefault radius(0.5);

  svtkm::cont::DataSet ds = MakeTestDatasetStructured();

  bool invertClip = true;
  svtkm::worklet::Clip clip;
  svtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(),
             svtkm::cont::make_ImplicitFunctionHandle<svtkm::Sphere>(center, radius),
             ds.GetCoordinateSystem("coords"),
             invertClip);

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  svtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> scalars = clip.ProcessPointField(scalarsIn);

  svtkm::cont::ArrayHandle<svtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  svtkm::cont::ArrayHandle<svtkm::Float32> cellvar = clip.ProcessCellField(cellvarIn);

  svtkm::Id connectivitySize = 12;
  svtkm::Id fieldSize = 13;
  svtkm::Id expectedConnectivity[] = { 10, 9, 4, 9, 11, 4, 12, 10, 4, 11, 12, 4 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f),  Coord3D(1.0f, 0.0f, 0.0f),  Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f),  Coord3D(1.0f, 1.0f, 0.0f),  Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f),  Coord3D(1.0f, 2.0f, 0.0f),  Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.75f, 0.0f), Coord3D(0.75f, 1.0f, 0.0f), Coord3D(1.25f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.25f, 0.0f),
  };
  svtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  std::vector<svtkm::Float32> expectedCellvar = { -100.f, 100.f, 30.f, -30.f };

  SVTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(svtkm::TopologyElementTagCell(),
                                                       svtkm::TopologyElementTagPoint()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  SVTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  SVTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  SVTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<svtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

void TestClipping()
{
  std::cout << "Testing explicit dataset:" << std::endl;
  TestClippingExplicit();
  std::cout << "Testing structured dataset:" << std::endl;
  TestClippingStructured();
  std::cout << "Testing clipping with implicit function (sphere):" << std::endl;
  TestClippingWithImplicitFunction();
  TestClippingWithImplicitFunctionInverted();
}

int UnitTestClipping(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestClipping, argc, argv);
}
