//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/testing/ExplicitTestData.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace DataSetBuilderExplicitNamespace
{

using DFA = svtkm::cont::Algorithm;

template <typename T>
svtkm::Bounds ComputeBounds(std::size_t numPoints, const T* coords)
{
  svtkm::Bounds bounds;

  for (std::size_t i = 0; i < numPoints; i++)
  {
    bounds.Include(svtkm::Vec<T, 3>(coords[i * 3 + 0], coords[i * 3 + 1], coords[i * 3 + 2]));
  }

  return bounds;
}

void ValidateDataSet(const svtkm::cont::DataSet& ds,
                     svtkm::Id numPoints,
                     svtkm::Id numCells,
                     const svtkm::Bounds& bounds)
{
  //Verify basics..
  SVTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Wrong number of fields.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1, "Wrong number of coordinate systems.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numPoints, "Wrong number of coordinates.");
  SVTKM_TEST_ASSERT(ds.GetNumberOfCells() == numCells, "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    ds.GetPointField("pointvar");
  }
  catch (...)
  {
    SVTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure bounds are correct.
  svtkm::Bounds computedBounds = ds.GetCoordinateSystem().GetBounds();
  SVTKM_TEST_ASSERT(test_equal(bounds, computedBounds), "Bounds of coordinates do not match");
}

template <typename T>
std::vector<T> createVec(std::size_t n, const T* data)
{
  std::vector<T> vec(n);
  for (std::size_t i = 0; i < n; i++)
  {
    vec[i] = data[i];
  }
  return vec;
}

template <typename T>
svtkm::cont::ArrayHandle<T> createAH(std::size_t n, const T* data)
{
  svtkm::cont::ArrayHandle<T> arr;
  DFA::Copy(svtkm::cont::make_ArrayHandle(data, static_cast<svtkm::Id>(n)), arr);
  return arr;
}

template <typename T>
svtkm::cont::DataSet CreateDataSetArr(bool useSeparatedCoords,
                                     std::size_t numPoints,
                                     const T* coords,
                                     std::size_t numCells,
                                     std::size_t numConn,
                                     const svtkm::Id* conn,
                                     const svtkm::IdComponent* indices,
                                     const svtkm::UInt8* shape)
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetFieldAdd dsf;
  svtkm::cont::DataSetBuilderExplicit dsb;
  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> xvals(numPoints), yvals(numPoints), zvals(numPoints);
    std::vector<T> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      xvals[i] = coords[i * 3 + 0];
      yvals[i] = coords[i * 3 + 1];
      zvals[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
    }
    svtkm::cont::ArrayHandle<T> X, Y, Z, P, C;
    DFA::Copy(svtkm::cont::make_ArrayHandle(xvals), X);
    DFA::Copy(svtkm::cont::make_ArrayHandle(yvals), Y);
    DFA::Copy(svtkm::cont::make_ArrayHandle(zvals), Z);
    DFA::Copy(svtkm::cont::make_ArrayHandle(varP), P);
    DFA::Copy(svtkm::cont::make_ArrayHandle(varC), C);
    dataSet = dsb.Create(
      X, Y, Z, createAH(numCells, shape), createAH(numCells, indices), createAH(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", P);
    dsf.AddCellField(dataSet, "cellvar", C);
    return dataSet;
  }
  else
  {
    std::vector<svtkm::Vec<T, 3>> tmp(numPoints);
    std::vector<svtkm::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      tmp[i][0] = coords[i * 3 + 0];
      tmp[i][1] = coords[i * 3 + 1];
      tmp[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> pts;
    DFA::Copy(svtkm::cont::make_ArrayHandle(tmp), pts);
    dataSet = dsb.Create(
      pts, createAH(numCells, shape), createAH(numCells, indices), createAH(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
}

template <typename T>
svtkm::cont::DataSet CreateDataSetVec(bool useSeparatedCoords,
                                     std::size_t numPoints,
                                     const T* coords,
                                     std::size_t numCells,
                                     std::size_t numConn,
                                     const svtkm::Id* conn,
                                     const svtkm::IdComponent* indices,
                                     const svtkm::UInt8* shape)
{
  svtkm::cont::DataSet dataSet;
  svtkm::cont::DataSetFieldAdd dsf;
  svtkm::cont::DataSetBuilderExplicit dsb;

  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> X(numPoints), Y(numPoints), Z(numPoints), varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      X[i] = coords[i * 3 + 0];
      Y[i] = coords[i * 3 + 1];
      Z[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      X, Y, Z, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
  else
  {
    std::vector<svtkm::Vec<T, 3>> pts(numPoints);
    std::vector<svtkm::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      pts[i][0] = coords[i * 3 + 0];
      pts[i][1] = coords[i * 3 + 1];
      pts[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      pts, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
}

#define TEST_DATA(num)                                                                             \
  svtkm::cont::testing::ExplicitData##num::numPoints,                                               \
    svtkm::cont::testing::ExplicitData##num::coords,                                                \
    svtkm::cont::testing::ExplicitData##num::numCells,                                              \
    svtkm::cont::testing::ExplicitData##num::numConn, svtkm::cont::testing::ExplicitData##num::conn, \
    svtkm::cont::testing::ExplicitData##num::numIndices,                                            \
    svtkm::cont::testing::ExplicitData##num::shapes
#define TEST_NUMS(num)                                                                             \
  svtkm::cont::testing::ExplicitData##num::numPoints,                                               \
    svtkm::cont::testing::ExplicitData##num::numCells
#define TEST_BOUNDS(num)                                                                           \
  svtkm::cont::testing::ExplicitData##num::numPoints, svtkm::cont::testing::ExplicitData##num::coords

void TestDataSetBuilderExplicit()
{
  svtkm::cont::DataSetBuilderExplicit dsb;
  svtkm::cont::DataSet ds;
  svtkm::Bounds bounds;

  //Iterate over organization of coordinates.
  for (int i = 0; i < 2; i++)
  {
    //Test ExplicitData0
    bounds = ComputeBounds(TEST_BOUNDS(0));
    ds = CreateDataSetArr(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);

    //Test ExplicitData1
    bounds = ComputeBounds(TEST_BOUNDS(1));
    ds = CreateDataSetArr(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);

    //Test ExplicitData2
    bounds = ComputeBounds(TEST_BOUNDS(2));
    ds = CreateDataSetArr(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
  }
}

} // namespace DataSetBuilderExplicitNamespace

int UnitTestDataSetBuilderExplicit(int argc, char* argv[])
{
  using namespace DataSetBuilderExplicitNamespace;
  return svtkm::cont::testing::Testing::Run(TestDataSetBuilderExplicit, argc, argv);
}
